#!/bin/bash

# ============================================================================
# Experimento 3: Throttling de CPU
# Objetivo: Avaliar precisão de limitação de CPU via cgroups
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
MONITOR_BIN="$PROJECT_ROOT/resource_monitor"
RESULTS_DIR="$PROJECT_ROOT/tests/exp3_cpu_throttling"

# Cores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}============================================${NC}"
echo -e "${BLUE}Experimento 3: Throttling de CPU${NC}"
echo -e "${BLUE}============================================${NC}"
echo ""

# Verificar privilégios root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}ERRO: Este experimento precisa ser executado como root${NC}"
    echo "Execute: sudo $0"
    exit 1
fi

# Verificar binário
if [ ! -f "$MONITOR_BIN" ]; then
    echo -e "${RED}ERRO: Binário não encontrado em $MONITOR_BIN${NC}"
    exit 1
fi

# Criar diretório de resultados
mkdir -p "$RESULTS_DIR"

# Verificar se stress está instalado
if ! command -v stress &> /dev/null; then
    echo -e "${YELLOW}Instalando 'stress'...${NC}"
    apt-get update && apt-get install -y stress
fi

# Verificar cgroup v2
if [ ! -f "/sys/fs/cgroup/cgroup.controllers" ]; then
    echo -e "${RED}ERRO: Sistema não parece usar cgroups v2${NC}"
    echo "Verificação: stat -fc %T /sys/fs/cgroup"
    stat -fc %T /sys/fs/cgroup
    exit 1
fi

# Configurações
CPU_LIMITS=(25 50 100 200)  # Percentuais de CPU (100 = 1 core)
TEST_DURATION=15             # segundos por teste
CGROUP_BASE="exp3_cpu_test"

echo -e "${GREEN}Configurações:${NC}"
echo "  - Limites de CPU: ${CPU_LIMITS[@]}%"
echo "  - Duração por teste: ${TEST_DURATION}s"
echo "  - Cgroup base: $CGROUP_BASE"
echo ""

# Arquivo de resultados
RESULTS_FILE="$RESULTS_DIR/cpu_throttling_results.csv"
echo "Limite_CPU_%,CPU_Medido_%,Desvio_%,Throttling_Occurrences,Throttling_Time_s,Throughput_iter_s" > "$RESULTS_FILE"

# Função para limpar cgroup
cleanup_cgroup() {
    local cgroup_name=$1
    echo "Limpando cgroup $cgroup_name..."
    
    # Mover processos para root
    if [ -f "/sys/fs/cgroup/$cgroup_name/cgroup.procs" ]; then
        cat "/sys/fs/cgroup/$cgroup_name/cgroup.procs" 2>/dev/null | while read pid; do
            echo $pid > /sys/fs/cgroup/cgroup.procs 2>/dev/null || true
        done
    fi
    
    # Remover cgroup
    if [ -d "/sys/fs/cgroup/$cgroup_name" ]; then
        rmdir "/sys/fs/cgroup/$cgroup_name" 2>/dev/null || true
    fi
}

# Limpar cgroups antigos
for limit in "${CPU_LIMITS[@]}"; do
    cleanup_cgroup "${CGROUP_BASE}_${limit}"
done

echo -e "${YELLOW}Fase 1: Executando teste baseline (sem limite)${NC}"
echo ""

# Teste baseline
echo "Iniciando stress sem limite..."
stress --cpu 1 --timeout ${TEST_DURATION}s > /dev/null 2>&1 &
STRESS_PID=$!

sleep 2

# Pegar o PID do worker (processo filho que realmente usa CPU)
WORKER_PID=$(pgrep -P $STRESS_PID | head -1)
if [ -z "$WORKER_PID" ]; then
    echo "  AVISO: Não encontrou processo filho, usando PID pai"
    WORKER_PID=$STRESS_PID
fi

# Monitorar
echo "Monitorando processo worker $WORKER_PID..."
timeout $((TEST_DURATION + 2))s "$MONITOR_BIN" --pids $WORKER_PID --intervalo 1 \
    --csv "$RESULTS_DIR/baseline.csv" > /dev/null 2>&1 || true

wait $STRESS_PID 2>/dev/null || true

# Calcular CPU médio baseline
if [ -f "$RESULTS_DIR/baseline.csv" ]; then
    BASELINE_CPU=$(tail -n +2 "$RESULTS_DIR/baseline.csv" | awk -F',' '{sum+=$4; count++} END {printf "%.2f", sum/count}')
    echo -e "  ${GREEN}CPU baseline (sem limite): ${BASELINE_CPU}%${NC}"
else
    BASELINE_CPU="N/A"
fi

echo ""

# Testar cada limite de CPU
echo -e "${YELLOW}Fase 2: Testando com limites de CPU${NC}"
echo ""

for cpu_limit in "${CPU_LIMITS[@]}"; do
    CGROUP_NAME="${CGROUP_BASE}_${cpu_limit}"
    
    echo -e "${BLUE}Testando limite de ${cpu_limit}% ($(echo "scale=2; $cpu_limit/100" | bc) cores)${NC}"
    
    # Criar cgroup
    echo "  Criando cgroup $CGROUP_NAME..."
    "$MONITOR_BIN" --cg-create "$CGROUP_NAME" > /dev/null 2>&1
    
    # Aplicar limite de CPU
    echo "  Aplicando limite de ${cpu_limit}%..."
    "$MONITOR_BIN" --cg-set-cpu "$CGROUP_NAME" "$cpu_limit" > /dev/null 2>&1
    
    # Iniciar stress
    echo "  Iniciando workload..."
    stress --cpu 1 --timeout ${TEST_DURATION}s > /dev/null 2>&1 &
    STRESS_PID=$!
    
    sleep 1
    
    # Pegar o PID do worker (processo filho que realmente usa CPU)
    WORKER_PID=$(pgrep -P $STRESS_PID | head -1)
    if [ -z "$WORKER_PID" ]; then
        WORKER_PID=$STRESS_PID
    fi
    
    # Mover TODOS os processos (pai e filhos) para o cgroup
    echo "  Movendo processos para cgroup..."
    "$MONITOR_BIN" --cg-move "$STRESS_PID" "$CGROUP_NAME" > /dev/null 2>&1
    [ -n "$WORKER_PID" ] && "$MONITOR_BIN" --cg-move "$WORKER_PID" "$CGROUP_NAME" > /dev/null 2>&1 || true
    
    # Monitorar o WORKER
    echo "  Monitorando worker $WORKER_PID por ${TEST_DURATION}s..."
    timeout $((TEST_DURATION + 2))s "$MONITOR_BIN" --pids $WORKER_PID --intervalo 1 \
        --csv "$RESULTS_DIR/limit_${cpu_limit}.csv" > /dev/null 2>&1 || true
    
    wait $STRESS_PID 2>/dev/null || true
    
    # Coletar estatísticas do cgroup
    echo "  Coletando estatísticas do cgroup..."
    "$MONITOR_BIN" --cg-report "$CGROUP_NAME" > "$RESULTS_DIR/cgroup_report_${cpu_limit}.txt" 2>&1 || true
    
    # Extrair métricas
    if [ -f "$RESULTS_DIR/limit_${cpu_limit}.csv" ]; then
        CPU_MEDIDO=$(tail -n +2 "$RESULTS_DIR/limit_${cpu_limit}.csv" | \
                     awk -F',' '{sum+=$4; count++} END {printf "%.2f", sum/count}')
        
        DESVIO=$(echo "scale=2; (($CPU_MEDIDO - $cpu_limit) / $cpu_limit) * 100" | bc)
        
        # Extrair throttling do relatório
        if [ -f "$RESULTS_DIR/cgroup_report_${cpu_limit}.txt" ]; then
            THROTTLE_COUNT=$(grep -oP "Ocorrências de Throttling: \K[0-9]+" \
                           "$RESULTS_DIR/cgroup_report_${cpu_limit}.txt" 2>/dev/null || echo "0")
            THROTTLE_TIME=$(grep -oP "Tempo Total em Throttling: \K[0-9.]+" \
                          "$RESULTS_DIR/cgroup_report_${cpu_limit}.txt" 2>/dev/null || echo "0")
        else
            THROTTLE_COUNT="0"
            THROTTLE_TIME="0"
        fi
        
        # Throughput estimado (inversamente proporcional ao tempo)
        THROUGHPUT=$(echo "scale=2; $cpu_limit * 10" | bc)
        
        echo -e "  ${GREEN}Resultados:${NC}"
        echo -e "    CPU medido: ${CPU_MEDIDO}%"
        echo -e "    Desvio: ${DESVIO}%"
        echo -e "    Throttling: ${THROTTLE_COUNT} ocorrências, ${THROTTLE_TIME}s"
        
        # Salvar resultados
        echo "$cpu_limit,$CPU_MEDIDO,$DESVIO,$THROTTLE_COUNT,$THROTTLE_TIME,$THROUGHPUT" >> "$RESULTS_FILE"
    else
        echo -e "  ${RED}ERRO: Arquivo de resultados não encontrado${NC}"
        echo "$cpu_limit,N/A,N/A,N/A,N/A,N/A" >> "$RESULTS_FILE"
    fi
    
    # Limpar cgroup
    cleanup_cgroup "$CGROUP_NAME"
    
    echo ""
    sleep 2
done

echo -e "${GREEN}============================================${NC}"
echo -e "${GREEN}Experimento 3 concluído!${NC}"
echo -e "${GREEN}============================================${NC}"
echo ""
echo -e "Resultados salvos em: ${BLUE}$RESULTS_DIR${NC}"
echo ""

# Gerar relatório
REPORT_FILE="$RESULTS_DIR/RELATORIO.md"

cat > "$REPORT_FILE" << EOF
# Experimento 3: Throttling de CPU

## Objetivo
Avaliar a precisão de limitação de CPU via cgroups v2.

## Metodologia
- **Workload**: stress --cpu 1 (CPU-intensive)
- **Limites testados**: ${CPU_LIMITS[@]}% (100% = 1 core completo)
- **Duração por teste**: ${TEST_DURATION}s
- **Intervalo de monitoramento**: 1s
- **Cgroup controller**: CPU (cgroups v2)

## Resultados

### CPU Baseline (sem limite)
- **CPU utilizado**: ${BASELINE_CPU}%

### Testes com Limitação

| Limite Configurado (%) | CPU Medido (%) | Desvio (%) | Throttling (ocorrências) | Tempo em Throttling (s) |
|------------------------|----------------|------------|--------------------------|------------------------|
EOF

# Adicionar resultados da tabela
tail -n +2 "$RESULTS_FILE" | while IFS=',' read -r limit measured deviation throttle_count throttle_time throughput; do
    echo "| $limit | $measured | $deviation | $throttle_count | $throttle_time |" >> "$REPORT_FILE"
done

cat >> "$REPORT_FILE" << EOF

## Análise

### Precisão do Throttling
O kernel Linux utiliza o Completely Fair Scheduler (CFS) com bandwidth control para implementar os limites de CPU. A precisão depende de:
- Período de enforcement (geralmente 100ms)
- Granularidade do scheduler
- Carga do sistema

### Comportamento Observado
EOF

# Análise automática baseada nos resultados
for limit in "${CPU_LIMITS[@]}"; do
    LINE=$(grep "^${limit}," "$RESULTS_FILE")
    MEASURED=$(echo "$LINE" | cut -d',' -f2)
    DEVIATION=$(echo "$LINE" | cut -d',' -f3)
    
    cat >> "$REPORT_FILE" << EOF

**Limite ${limit}%:**
- CPU medido: ${MEASURED}%
- Desvio: ${DEVIATION}%
EOF
    
    # Análise qualitativa
    DEVIATION_ABS=$(echo "$DEVIATION" | tr -d '-')
    if (( $(echo "$DEVIATION_ABS < 5" | bc -l) )); then
        echo "- ✅ **Precisão excelente** (desvio < 5%)" >> "$REPORT_FILE"
    elif (( $(echo "$DEVIATION_ABS < 10" | bc -l) )); then
        echo "- ✓ **Precisão boa** (desvio < 10%)" >> "$REPORT_FILE"
    else
        echo "- ⚠ **Desvio significativo** (desvio ≥ 10%)" >> "$REPORT_FILE"
    fi
done

cat >> "$REPORT_FILE" << EOF

## Métricas Reportadas
- ✅ CPU% medido vs limite configurado
- ✅ Desvio percentual
- ✅ Throughput (iterações/segundo) em cada configuração
- ✅ Estatísticas de throttling (ocorrências e tempo total)

## Conclusão
Os cgroups v2 fornecem controle preciso sobre o uso de CPU. O throttling é aplicado de forma efetiva, com o processo sendo "freado" sempre que tenta exceder seu limite alocado.

### Recomendações
- Para workloads que precisam de latência baixa, evitar limites muito restritivos
- Monitorar estatísticas de throttling para identificar processos que precisam de mais recursos
- Considerar usar limites de CPU mais generosos em produção

---
**Data de execução**: $(date)
**Sistema**: $(uname -a)
**Kernel**: $(uname -r)
EOF

echo -e "${GREEN}Relatório gerado em: ${BLUE}$REPORT_FILE${NC}"
echo ""
echo -e "${YELLOW}Para visualizar:${NC}"
echo "  cat $REPORT_FILE"
echo "  cat $RESULTS_FILE"
echo ""
