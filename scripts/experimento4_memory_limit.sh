#!/bin/bash

# ============================================================================
# Experimento 4: Limitação de Memória
# Objetivo: Testar comportamento ao atingir limite de memória
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
MONITOR_BIN="$PROJECT_ROOT/resource_monitor"
RESULTS_DIR="$PROJECT_ROOT/tests/exp4_memory_limit"

# Cores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}============================================${NC}"
echo -e "${BLUE}Experimento 4: Limitação de Memória${NC}"
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

# Configurações
MEMORY_LIMIT_MB=100
CGROUP_NAME="exp4_memory_test"
ALLOCATION_STEP_MB=10

echo -e "${GREEN}Configurações:${NC}"
echo "  - Limite de memória: ${MEMORY_LIMIT_MB}MB"
echo "  - Step de alocação: ${ALLOCATION_STEP_MB}MB"
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

# Limpar cgroup anterior se existir
cleanup_cgroup "$CGROUP_NAME"

# Criar programa de teste de alocação de memória
TEST_PROGRAM="$RESULTS_DIR/memory_allocator"

cat > "${TEST_PROGRAM}.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MB (1024 * 1024)

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <step_mb> <max_mb>\n", argv[0]);
        return 1;
    }
    
    int step_mb = atoi(argv[1]);
    int max_mb = atoi(argv[2]);
    int allocated_mb = 0;
    
    printf("PID: %d\n", getpid());
    printf("Iniciando alocação de memória...\n");
    fflush(stdout);
    
    // Esperar ser movido para o cgroup
    sleep(2);
    
    while (allocated_mb < max_mb) {
        char *mem = (char *)malloc(step_mb * MB);
        if (mem == NULL) {
            printf("FALHA: malloc() retornou NULL após %d MB\n", allocated_mb);
            fflush(stdout);
            break;
        }
        
        // Garantir que a memória é realmente alocada (não apenas virtual)
        memset(mem, 1, step_mb * MB);
        
        allocated_mb += step_mb;
        printf("Alocado: %d MB\n", allocated_mb);
        fflush(stdout);
        
        sleep(1);
    }
    
    printf("Total alocado: %d MB\n", allocated_mb);
    printf("Mantendo alocação por 5 segundos...\n");
    fflush(stdout);
    sleep(5);
    
    return 0;
}
EOF

echo -e "${YELLOW}Compilando programa de teste...${NC}"
gcc -o "$TEST_PROGRAM" "${TEST_PROGRAM}.c"
echo ""

# Arquivo de resultados
RESULTS_FILE="$RESULTS_DIR/memory_results.txt"

echo -e "${YELLOW}Fase 1: Criando e configurando cgroup${NC}"
echo ""

# Criar cgroup
echo "Criando cgroup $CGROUP_NAME..."
"$MONITOR_BIN" --cg-create "$CGROUP_NAME"

# Aplicar limite de memória
echo "Aplicando limite de ${MEMORY_LIMIT_MB}MB..."
"$MONITOR_BIN" --cg-set-mem "$CGROUP_NAME" "$MEMORY_LIMIT_MB"

echo ""
echo -e "${YELLOW}Fase 2: Executando teste de alocação${NC}"
echo ""

# Executar programa de teste
echo "Iniciando alocação incremental de memória..."
"$TEST_PROGRAM" $ALLOCATION_STEP_MB $((MEMORY_LIMIT_MB * 2)) > "$RESULTS_DIR/allocator_output.txt" 2>&1 &
ALLOCATOR_PID=$!

sleep 1

# Obter PID do programa
ACTUAL_PID=$(grep "PID:" "$RESULTS_DIR/allocator_output.txt" 2>/dev/null | awk '{print $2}')
if [ -z "$ACTUAL_PID" ]; then
    ACTUAL_PID=$ALLOCATOR_PID
fi

echo "PID do processo: $ACTUAL_PID"

# Mover para cgroup
echo "Movendo processo para cgroup..."
"$MONITOR_BIN" --cg-move "$ACTUAL_PID" "$CGROUP_NAME"

# Monitorar o processo
echo "Monitorando uso de memória..."
timeout 60s "$MONITOR_BIN" --pids $ACTUAL_PID --intervalo 1 --verbose \
    --csv "$RESULTS_DIR/memory_usage.csv" > "$RESULTS_DIR/monitor_output.txt" 2>&1 || true

# Esperar o processo terminar
wait $ALLOCATOR_PID 2>/dev/null || true

echo ""
echo -e "${YELLOW}Fase 3: Coletando estatísticas do cgroup${NC}"
echo ""

# Gerar relatório do cgroup
"$MONITOR_BIN" --cg-report "$CGROUP_NAME" > "$RESULTS_DIR/cgroup_report.txt" 2>&1 || true

# Extrair métricas importantes
if [ -f "$RESULTS_DIR/cgroup_report.txt" ]; then
    echo "Estatísticas do cgroup:" | tee "$RESULTS_FILE"
    echo "" | tee -a "$RESULTS_FILE"
    
    # Memory usage
    grep -A 5 "\[Memory\]" "$RESULTS_DIR/cgroup_report.txt" | tee -a "$RESULTS_FILE"
    echo "" | tee -a "$RESULTS_FILE"
    
    # Extrair valores específicos
    CURRENT_MEM=$(grep "Uso Atual:" "$RESULTS_DIR/cgroup_report.txt" | awk '{print $3}')
    MAX_MEM=$(grep "Uso Máximo:" "$RESULTS_DIR/cgroup_report.txt" | awk '{print $3}')
    FAIL_COUNT=$(grep "Falhas de Alocação:" "$RESULTS_DIR/cgroup_report.txt" | awk '{print $4}')
    
    echo "Resumo:" | tee -a "$RESULTS_FILE"
    echo "  - Limite configurado: ${MEMORY_LIMIT_MB}MB" | tee -a "$RESULTS_FILE"
    echo "  - Uso máximo atingido: ${MAX_MEM:-N/A}" | tee -a "$RESULTS_FILE"
    echo "  - Falhas de alocação: ${FAIL_COUNT:-N/A}" | tee -a "$RESULTS_FILE"
else
    echo "ERRO: Relatório do cgroup não encontrado" | tee "$RESULTS_FILE"
fi

echo ""

# Analisar output do allocator
if [ -f "$RESULTS_DIR/allocator_output.txt" ]; then
    echo "Output do programa de teste:" | tee -a "$RESULTS_FILE"
    cat "$RESULTS_DIR/allocator_output.txt" | tee -a "$RESULTS_FILE"
    echo "" | tee -a "$RESULTS_FILE"
    
    # Encontrar quanto foi alocado antes da falha
    MAX_ALLOCATED=$(grep "Alocado:" "$RESULTS_DIR/allocator_output.txt" | tail -1 | awk '{print $2}')
    
    if [ -n "$MAX_ALLOCATED" ]; then
        echo "Quantidade máxima alocada com sucesso: ${MAX_ALLOCATED}MB" | tee -a "$RESULTS_FILE"
    fi
fi

# Verificar se houve OOM kill
echo "" | tee -a "$RESULTS_FILE"
echo "Verificando logs do kernel para OOM events..." | tee -a "$RESULTS_FILE"
dmesg | grep -i "killed process\|out of memory\|oom" | tail -5 | tee -a "$RESULTS_FILE" || \
    echo "Nenhum OOM event recente encontrado" | tee -a "$RESULTS_FILE"

echo ""

# Limpar cgroup
cleanup_cgroup "$CGROUP_NAME"

echo -e "${GREEN}============================================${NC}"
echo -e "${GREEN}Experimento 4 concluído!${NC}"
echo -e "${GREEN}============================================${NC}"
echo ""
echo -e "Resultados salvos em: ${BLUE}$RESULTS_DIR${NC}"
echo ""

# Gerar relatório final
REPORT_FILE="$RESULTS_DIR/RELATORIO.md"

cat > "$REPORT_FILE" << EOF
# Experimento 4: Limitação de Memória

## Objetivo
Testar o comportamento do sistema ao atingir o limite de memória configurado via cgroups.

## Metodologia
1. Criar cgroup com limite de ${MEMORY_LIMIT_MB}MB
2. Executar programa que aloca memória incrementalmente (${ALLOCATION_STEP_MB}MB por vez)
3. Observar comportamento ao atingir o limite
4. Coletar estatísticas de falhas de alocação

### Programa de Teste
- Alocação incremental de ${ALLOCATION_STEP_MB}MB por iteração
- Memset() para garantir alocação física (não apenas virtual)
- Tentativa de alocar até $((MEMORY_LIMIT_MB * 2))MB

## Resultados

### Configuração do Cgroup
- **Limite configurado**: ${MEMORY_LIMIT_MB}MB
- **Controller**: memory (cgroups v2)

### Comportamento Observado

EOF

# Adicionar resultados específicos
if [ -n "$MAX_ALLOCATED" ]; then
    cat >> "$REPORT_FILE" << EOF
**Quantidade máxima alocada**: ${MAX_ALLOCATED}MB

EOF
fi

if [ -n "$MAX_MEM" ]; then
    cat >> "$REPORT_FILE" << EOF
**Uso máximo reportado pelo cgroup**: ${MAX_MEM}

EOF
fi

if [ -n "$FAIL_COUNT" ]; then
    cat >> "$REPORT_FILE" << EOF
**Número de falhas de alocação (memory.failcnt)**: ${FAIL_COUNT}

EOF
fi

cat >> "$REPORT_FILE" << EOF
### Análise do Comportamento

Quando um processo tenta alocar memória além do limite do cgroup, o kernel pode:

1. **Recusar a alocação**: malloc() retorna NULL
2. **Invocar o OOM Killer**: Mata o processo para liberar memória
3. **Swap**: Mover páginas para swap (se disponível e configurado)

No experimento:
EOF

# Análise baseada nos resultados
if grep -q "FALHA" "$RESULTS_DIR/allocator_output.txt" 2>/dev/null; then
    cat >> "$REPORT_FILE" << EOF
- ✅ **Alocação foi recusada** - malloc() retornou NULL ao atingir o limite
- O programa tratou a falha graciosamente sem crash
EOF
fi

if grep -qi "killed process" "$RESULTS_DIR/cgroup_report.txt" 2>/dev/null || \
   dmesg | grep -qi "killed.*$ACTUAL_PID" 2>/dev/null; then
    cat >> "$REPORT_FILE" << EOF
- ⚠️ **OOM Killer foi invocado** - O processo foi terminado pelo kernel
EOF
fi

cat >> "$REPORT_FILE" << EOF

### Estatísticas do Cgroup

\`\`\`
$(cat "$RESULTS_DIR/cgroup_report.txt" 2>/dev/null || echo "Relatório não disponível")
\`\`\`

## Métricas Reportadas
- ✅ Quantidade máxima alocada
- ✅ Número de falhas (memory.failcnt)
- ✅ Comportamento do sistema ao atingir limite

## Conclusão

Os cgroups v2 fornecem controle efetivo sobre uso de memória. O limite é aplicado de forma rígida:
- Alocações que excedem o limite falham
- O kernel pode invocar o OOM killer se necessário
- Processos devem estar preparados para lidar com falhas de alocação

### Recomendações para Produção
1. **Configurar limites com margem**: Deixar 10-20% de margem acima do uso esperado
2. **Monitorar memory.failcnt**: Indica que o limite está muito baixo
3. **Implementar tratamento de erros**: Código deve verificar se malloc() retornou NULL
4. **Considerar memory.high**: Limit "soft" que causa throttling antes do OOM

---
**Data de execução**: $(date)
**Sistema**: $(uname -a)
**Memória total do sistema**: $(free -h | grep Mem | awk '{print $2}')
EOF

echo -e "${GREEN}Relatório gerado em: ${BLUE}$REPORT_FILE${NC}"
echo ""
echo -e "${YELLOW}Para visualizar:${NC}"
echo "  cat $REPORT_FILE"
echo "  cat $RESULTS_FILE"
echo ""
