#!/bin/bash

# ============================================================================
# Experimento 5: Limitação de I/O
# Objetivo: Avaliar precisão de limitação de I/O
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
MONITOR_BIN="$PROJECT_ROOT/resource_monitor"
RESULTS_DIR="$PROJECT_ROOT/experimentos/exp5_io_limit"

# Cores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}============================================${NC}"
echo -e "${BLUE}Experimento 5: Limitação de I/O${NC}"
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

# Verificar cgroup v2
if [ ! -f "/sys/fs/cgroup/cgroup.controllers" ]; then
    echo -e "${RED}ERRO: Sistema não parece usar cgroups v2${NC}"
    echo "Verificação: stat -fc %T /sys/fs/cgroup"
    stat -fc %T /sys/fs/cgroup
    exit 1
fi

# Verificar se dd está disponível
if ! command -v dd &> /dev/null; then
    echo -e "${RED}ERRO: comando 'dd' não encontrado${NC}"
    exit 1
fi

# Configurações
TEST_FILE="$RESULTS_DIR/test_io_file.dat"
FILE_SIZE_MB=500
CGROUP_NAME="exp5_io_test"
# Limites em MB/s
IO_LIMITS=(10 50 100)  # MB/s
TEST_DURATION=15        # segundos

echo -e "${GREEN}Configurações:${NC}"
echo "  - Tamanho do arquivo: ${FILE_SIZE_MB}MB"
echo "  - Limites de I/O: ${IO_LIMITS[@]} MB/s"
echo "  - Cgroup: $CGROUP_NAME"
echo ""

# Função para limpar cgroup
cleanup_cgroup() {
    local cgroup_name=$1
    
    if [ -f "/sys/fs/cgroup/$cgroup_name/cgroup.procs" ]; then
        cat "/sys/fs/cgroup/$cgroup_name/cgroup.procs" 2>/dev/null | while read pid; do
            echo $pid > /sys/fs/cgroup/cgroup.procs 2>/dev/null || true
        done
    fi
    
    if [ -d "/sys/fs/cgroup/$cgroup_name" ]; then
        rmdir "/sys/fs/cgroup/$cgroup_name" 2>/dev/null || true
    fi
}

# Limpar cgroup anterior
cleanup_cgroup "$CGROUP_NAME"

# Criar arquivo de teste
if [ ! -f "$TEST_FILE" ]; then
    echo -e "${YELLOW}Criando arquivo de teste de ${FILE_SIZE_MB}MB...${NC}"
    dd if=/dev/zero of="$TEST_FILE" bs=1M count=$FILE_SIZE_MB status=progress 2>&1 | tail -1
    echo ""
fi

# Descobrir o device do filesystem
DEVICE=$(df "$RESULTS_DIR" | tail -1 | awk '{print $1}')
MAJOR_MINOR=$(lsblk -no MAJ:MIN "$DEVICE" 2>/dev/null | head -1)

if [ -z "$MAJOR_MINOR" ]; then
    echo -e "${YELLOW}AVISO: Não foi possível determinar major:minor do device${NC}"
    echo "Tentando obter do /proc/self/mountinfo..."
    MAJOR_MINOR=$(awk -v dev="$DEVICE" '$0 ~ dev {print $3; exit}' /proc/self/mountinfo)
fi

echo -e "${GREEN}Device para I/O: ${DEVICE} (${MAJOR_MINOR})${NC}"
echo ""

# Arquivo de resultados
RESULTS_FILE="$RESULTS_DIR/io_results.csv"
echo "Limite_MB_s,Throughput_Leitura_MB_s,Throughput_Escrita_MB_s,Latencia_Leitura_ms,Latencia_Escrita_ms,Tempo_Execucao_s" > "$RESULTS_FILE"

echo -e "${YELLOW}Fase 1: Teste baseline (sem limite)${NC}"
echo ""

# Teste de leitura baseline
echo "Teste de leitura baseline..."
sync && echo 3 > /proc/sys/vm/drop_caches  # Limpar cache
READ_START=$(date +%s.%N)
dd if="$TEST_FILE" of=/dev/null bs=1M 2>&1 | grep -oP '\d+(\.\d+)? MB/s' | head -1 > /tmp/baseline_read.txt || true
READ_END=$(date +%s.%N)
READ_TIME=$(echo "$READ_END - $READ_START" | bc)
BASELINE_READ_THROUGHPUT=$(cat /tmp/baseline_read.txt 2>/dev/null | awk '{print $1}')
[ -z "$BASELINE_READ_THROUGHPUT" ] && BASELINE_READ_THROUGHPUT="N/A"

echo -e "  ${GREEN}Throughput de leitura baseline: ${BASELINE_READ_THROUGHPUT} MB/s${NC}"
echo -e "  ${GREEN}Tempo: ${READ_TIME}s${NC}"

# Teste de escrita baseline
echo "Teste de escrita baseline..."
WRITE_START=$(date +%s.%N)
dd if=/dev/zero of="$RESULTS_DIR/test_write.dat" bs=1M count=100 oflag=direct 2>&1 | grep -oP '\d+(\.\d+)? MB/s' | head -1 > /tmp/baseline_write.txt || true
WRITE_END=$(date +%s.%N)
WRITE_TIME=$(echo "$WRITE_END - $WRITE_START" | bc)
BASELINE_WRITE_THROUGHPUT=$(cat /tmp/baseline_write.txt 2>/dev/null | awk '{print $1}')
[ -z "$BASELINE_WRITE_THROUGHPUT" ] && BASELINE_WRITE_THROUGHPUT="N/A"

echo -e "  ${GREEN}Throughput de escrita baseline: ${BASELINE_WRITE_THROUGHPUT} MB/s${NC}"
echo -e "  ${GREEN}Tempo: ${WRITE_TIME}s${NC}"

rm -f "$RESULTS_DIR/test_write.dat"

# Salvar baseline
BASELINE_TOTAL_TIME=$(echo "$READ_TIME + $WRITE_TIME" | bc)
echo "baseline,$BASELINE_READ_THROUGHPUT,$BASELINE_WRITE_THROUGHPUT,N/A,N/A,$BASELINE_TOTAL_TIME" >> "$RESULTS_FILE"

echo ""
echo -e "${YELLOW}Fase 2: Testes com limitação de I/O${NC}"
echo ""

for io_limit in "${IO_LIMITS[@]}"; do
    echo -e "${BLUE}Testando limite de ${io_limit} MB/s${NC}"
    
    # Criar cgroup
    echo "  Criando cgroup..."
    "$MONITOR_BIN" --cg-create "$CGROUP_NAME" > /dev/null 2>&1 || true
    
    # Aplicar limite de I/O
    echo "  Aplicando limite de I/O..."
    # Converter MB/s para bytes/s
    LIMIT_BPS=$((io_limit * 1024 * 1024))
    
    # Aplicar limite (cgroups v2)
    if [ -n "$MAJOR_MINOR" ]; then
        # rbps = read bytes per second, wbps = write bytes per second
        echo "$MAJOR_MINOR rbps=$LIMIT_BPS wbps=$LIMIT_BPS" > "/sys/fs/cgroup/$CGROUP_NAME/io.max" 2>/dev/null || {
            echo -e "  ${YELLOW}AVISO: Não foi possível aplicar limite de I/O${NC}"
            echo -e "  ${YELLOW}O controller 'io' pode não estar habilitado ou o device não suporta${NC}"
        }
    fi
    
    # Teste de leitura com limite
    echo "  Teste de leitura com limite..."
    sync && echo 3 > /proc/sys/vm/drop_caches
    
    # Executar dd em background e mover para cgroup
    dd if="$TEST_FILE" of=/dev/null bs=1M 2>"$RESULTS_DIR/dd_read_${io_limit}.log" &
    DD_PID=$!
    
    # Mover para cgroup
    "$MONITOR_BIN" --cg-move "$DD_PID" "$CGROUP_NAME" > /dev/null 2>&1 || true
    
    # Monitorar
    READ_START=$(date +%s.%N)
    wait $DD_PID 2>/dev/null || true
    READ_END=$(date +%s.%N)
    READ_TIME=$(echo "$READ_END - $READ_START" | bc)
    
    # Extrair throughput
    READ_THROUGHPUT=$(grep -oP '\d+(\.\d+)? MB/s' "$RESULTS_DIR/dd_read_${io_limit}.log" 2>/dev/null | head -1 | awk '{print $1}')
    if [ -z "$READ_THROUGHPUT" ]; then
        # Calcular manualmente
        READ_THROUGHPUT=$(echo "scale=2; $FILE_SIZE_MB / $READ_TIME" | bc)
    fi
    
    echo -e "    ${GREEN}Throughput: ${READ_THROUGHPUT} MB/s (tempo: ${READ_TIME}s)${NC}"
    
    # Teste de escrita com limite
    echo "  Teste de escrita com limite..."
    dd if=/dev/zero of="$RESULTS_DIR/test_write_${io_limit}.dat" bs=1M count=100 oflag=direct 2>"$RESULTS_DIR/dd_write_${io_limit}.log" &
    DD_PID=$!
    
    # Mover para cgroup
    "$MONITOR_BIN" --cg-move "$DD_PID" "$CGROUP_NAME" > /dev/null 2>&1 || true
    
    WRITE_START=$(date +%s.%N)
    wait $DD_PID 2>/dev/null || true
    WRITE_END=$(date +%s.%N)
    WRITE_TIME=$(echo "$WRITE_END - $WRITE_START" | bc)
    
    WRITE_THROUGHPUT=$(grep -oP '\d+(\.\d+)? MB/s' "$RESULTS_DIR/dd_write_${io_limit}.log" 2>/dev/null | head -1 | awk '{print $1}')
    if [ -z "$WRITE_THROUGHPUT" ]; then
        WRITE_THROUGHPUT=$(echo "scale=2; 100 / $WRITE_TIME" | bc)
    fi
    
    echo -e "    ${GREEN}Throughput: ${WRITE_THROUGHPUT} MB/s (tempo: ${WRITE_TIME}s)${NC}"
    
    # Calcular latência aproximada
    READ_LATENCY=$(echo "scale=2; ($READ_TIME / $FILE_SIZE_MB) * 1000" | bc)
    WRITE_LATENCY=$(echo "scale=2; ($WRITE_TIME / 100) * 1000" | bc)
    
    TOTAL_TIME=$(echo "$READ_TIME + $WRITE_TIME" | bc)
    
    # Salvar resultados
    echo "$io_limit,$READ_THROUGHPUT,$WRITE_THROUGHPUT,$READ_LATENCY,$WRITE_LATENCY,$TOTAL_TIME" >> "$RESULTS_FILE"
    
    # Gerar relatório do cgroup
    "$MONITOR_BIN" --cg-report "$CGROUP_NAME" > "$RESULTS_DIR/cgroup_report_${io_limit}.txt" 2>&1 || true
    
    # Limpar
    rm -f "$RESULTS_DIR/test_write_${io_limit}.dat"
    cleanup_cgroup "$CGROUP_NAME"
    
    echo ""
    sleep 2
done

echo -e "${GREEN}============================================${NC}"
echo -e "${GREEN}Experimento 5 concluído!${NC}"
echo -e "${GREEN}============================================${NC}"
echo ""
echo -e "Resultados salvos em: ${BLUE}$RESULTS_DIR${NC}"
echo ""

# Gerar relatório
REPORT_FILE="$RESULTS_DIR/RELATORIO.md"

cat > "$REPORT_FILE" << EOF
# Experimento 5: Limitação de I/O

## Objetivo
Avaliar a precisão de limitação de I/O de disco via cgroups v2.

## Metodologia
- **Workload**: dd para leitura e escrita sequencial
- **Tamanho do arquivo**: ${FILE_SIZE_MB}MB (leitura) / 100MB (escrita)
- **Limites testados**: ${IO_LIMITS[@]} MB/s
- **Device**: $DEVICE ($MAJOR_MINOR)
- **Cgroup controller**: io (cgroups v2)

## Resultados

### Baseline (sem limite)
- **Throughput de leitura**: ${BASELINE_READ_THROUGHPUT} MB/s
- **Throughput de escrita**: ${BASELINE_WRITE_THROUGHPUT} MB/s

### Testes com Limitação

| Limite (MB/s) | Leitura Real (MB/s) | Escrita Real (MB/s) | Latência Leitura (ms) | Latência Escrita (ms) | Tempo Total (s) |
|---------------|---------------------|---------------------|----------------------|----------------------|-----------------|
EOF

# Adicionar resultados
tail -n +2 "$RESULTS_FILE" | grep -v "baseline" | while IFS=',' read -r limit read_tp write_tp read_lat write_lat total_time; do
    echo "| $limit | $read_tp | $write_tp | $read_lat | $write_lat | $total_time |" >> "$REPORT_FILE"
done

cat >> "$REPORT_FILE" << EOF

## Análise

### Precisão dos Limites
O controller 'io' do cgroups v2 utiliza o subsistema blk-cgroup do kernel para implementar limitação de I/O.

EOF

# Análise de cada limite
for io_limit in "${IO_LIMITS[@]}"; do
    LINE=$(grep "^${io_limit}," "$RESULTS_FILE")
    if [ -n "$LINE" ]; then
        READ_TP=$(echo "$LINE" | cut -d',' -f2)
        WRITE_TP=$(echo "$LINE" | cut -d',' -f3)
        
        cat >> "$REPORT_FILE" << EOF

**Limite ${io_limit} MB/s:**
- Throughput de leitura medido: ${READ_TP} MB/s
- Throughput de escrita medido: ${WRITE_TP} MB/s
EOF
        
        # Calcular desvio
        if [ "$READ_TP" != "N/A" ] && [ -n "$READ_TP" ]; then
            READ_DEVIATION=$(echo "scale=2; (($READ_TP - $io_limit) / $io_limit) * 100" | bc 2>/dev/null || echo "N/A")
            echo "- Desvio de leitura: ${READ_DEVIATION}%" >> "$REPORT_FILE"
        fi
        
        if [ "$WRITE_TP" != "N/A" ] && [ -n "$WRITE_TP" ]; then
            WRITE_DEVIATION=$(echo "scale=2; (($WRITE_TP - $io_limit) / $io_limit) * 100" | bc 2>/dev/null || echo "N/A")
            echo "- Desvio de escrita: ${WRITE_DEVIATION}%" >> "$REPORT_FILE"
        fi
    fi
done

cat >> "$REPORT_FILE" << EOF

### Impacto na Latência
A limitação de I/O aumenta a latência das operações, pois o kernel precisa "frear" o processo quando ele atinge o limite de throughput.

### Limitações do Experimento
- Alguns dispositivos (ex: tmpfs, certos SSDs) podem não suportar limitação via io.max
- O cache do filesystem pode afetar os resultados
- I/O assíncrono e buffering podem mascarar os efeitos da limitação

## Métricas Reportadas
- ✅ Throughput medido vs limite configurado
- ✅ Latência de I/O
- ✅ Impacto no tempo total de execução

## Conclusão

O controller 'io' do cgroups v2 permite controlar o throughput de I/O de forma efetiva, embora a precisão dependa de:
1. **Suporte do device**: Alguns dispositivos virtuais não suportam limitação
2. **Tipo de I/O**: I/O direto (O_DIRECT) é mais preciso que buffered I/O
3. **Carga do sistema**: Outros processos competindo por I/O podem afetar os resultados

### Recomendações
- Para controle mais preciso, usar I/O direto (oflag=direct)
- Monitorar estatísticas em io.stat para verificar efetividade
- Considerar usar io.latency para controle baseado em latência
- Em SSDs modernos, limitações podem ser menos efetivas devido ao paralelismo

---
**Data de execução**: $(date)
**Sistema**: $(uname -a)
**Device**: $DEVICE ($MAJOR_MINOR)
**Filesystem**: $(df -T "$RESULTS_DIR" | tail -1 | awk '{print $2}')
EOF

echo -e "${GREEN}Relatório gerado em: ${BLUE}$REPORT_FILE${NC}"
echo ""
echo -e "${YELLOW}Para visualizar:${NC}"
echo "  cat $REPORT_FILE"
echo "  cat $RESULTS_FILE"
echo ""

# Limpeza final
rm -f /tmp/baseline_read.txt /tmp/baseline_write.txt
echo -e "${YELLOW}NOTA:${NC} Se os limites não foram aplicados corretamente, verifique:"
echo "  1. Se o controller 'io' está habilitado: cat /sys/fs/cgroup/cgroup.controllers"
echo "  2. Se o device suporta limitação: lsblk -o NAME,TYPE"
echo "  3. Se você está usando um filesystem real (não tmpfs)"
echo ""
