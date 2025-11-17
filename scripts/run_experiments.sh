#!/bin/bash

# ============================================================================
# Script Mestre - Execução de Todos os Experimentos Obrigatórios
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Cores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║                                                            ║${NC}"
echo -e "${CYAN}║  EXPERIMENTOS OBRIGATÓRIOS - CONTAINERS E RECURSOS         ║${NC}"
echo -e "${CYAN}║  Resource Monitor - Análise de Isolamento e Limitação     ║${NC}"
echo -e "${CYAN}║                                                            ║${NC}"
echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Verificar se está no diretório correto
if [ ! -f "$PROJECT_ROOT/resource_monitor" ]; then
    echo -e "${RED}ERRO: Executável não encontrado!${NC}"
    echo "Certifique-se de compilar o projeto primeiro:"
    echo "  cd $PROJECT_ROOT && make"
    exit 1
fi

# Menu de opções
show_menu() {
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}Escolha o experimento para executar:${NC}"
    echo ""
    echo "  1) Experimento 1: Overhead de Monitoramento"
    echo "  2) Experimento 2: Isolamento via Namespaces (requer sudo)"
    echo "  3) Experimento 3: Throttling de CPU (requer sudo)"
    echo "  4) Experimento 4: Limitação de Memória (requer sudo)"
    echo "  5) Experimento 5: Limitação de I/O (requer sudo)"
    echo ""
    echo "  6) Executar TODOS os experimentos (requer sudo)"
    echo "  7) Gerar relatório consolidado"
    echo ""
    echo "  0) Sair"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo -n "Opção: "
}

# Função para executar experimento
run_experiment() {
    local exp_num=$1
    local exp_script="$SCRIPT_DIR/experimento${exp_num}_*.sh"
    
    # Encontrar o script
    local script=$(ls $exp_script 2>/dev/null | head -1)
    
    if [ ! -f "$script" ]; then
        echo -e "${RED}ERRO: Script do experimento $exp_num não encontrado!${NC}"
        return 1
    fi
    
    # Tornar executável
    chmod +x "$script"
    
    echo -e "${YELLOW}Executando: $(basename $script)${NC}"
    echo ""
    
    # Executar
    if [[ "$exp_num" == "1" ]]; then
        # Experimento 1 não precisa de sudo
        bash "$script"
    else
        # Outros experimentos precisam de sudo
        if [ "$EUID" -ne 0 ]; then
            echo -e "${YELLOW}Este experimento precisa de privilégios root.${NC}"
            sudo bash "$script"
        else
            bash "$script"
        fi
    fi
    
    echo ""
    echo -e "${GREEN}✓ Experimento $exp_num concluído!${NC}"
    echo ""
    read -p "Pressione ENTER para continuar..."
}

# Função para executar todos
run_all() {
    echo -e "${CYAN}Executando todos os experimentos...${NC}"
    echo ""
    
    for i in {1..5}; do
        echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
        echo -e "${BLUE}Experimento $i de 5${NC}"
        echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
        run_experiment $i
        sleep 2
    done
    
    echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}✓ Todos os experimentos concluídos!${NC}"
    echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    
    # Gerar relatório consolidado
    generate_consolidated_report
}

# Função para gerar relatório consolidado
generate_consolidated_report() {
    echo -e "${YELLOW}Gerando relatório consolidado...${NC}"
    
    CONSOLIDATED_REPORT="$PROJECT_ROOT/experimentos/RELATORIO_CONSOLIDADO.md"
    
    cat > "$CONSOLIDATED_REPORT" << 'EOF'
# Relatório Consolidado dos Experimentos Obrigatórios

## Informações do Sistema

EOF
    
    # Adicionar informações do sistema
    cat >> "$CONSOLIDATED_REPORT" << EOF
- **Data de execução**: $(date)
- **Sistema Operacional**: $(cat /etc/os-release | grep PRETTY_NAME | cut -d'"' -f2)
- **Kernel**: $(uname -r)
- **Arquitetura**: $(uname -m)
- **CPU**: $(lscpu | grep "Model name" | cut -d':' -f2 | xargs)
- **Cores**: $(nproc)
- **Memória Total**: $(free -h | grep Mem | awk '{print $2}')
- **Cgroups Version**: $(stat -fc %T /sys/fs/cgroup)

---

EOF
    
    # Adicionar cada experimento
    for i in {1..5}; do
        local exp_dir="$PROJECT_ROOT/experimentos/exp${i}_"*
        local report_file=$(find $exp_dir -name "RELATORIO.md" 2>/dev/null | head -1)
        
        if [ -f "$report_file" ]; then
            echo "## Experimento $i" >> "$CONSOLIDATED_REPORT"
            echo "" >> "$CONSOLIDATED_REPORT"
            tail -n +2 "$report_file" >> "$CONSOLIDATED_REPORT"
            echo "" >> "$CONSOLIDATED_REPORT"
            echo "---" >> "$CONSOLIDATED_REPORT"
            echo "" >> "$CONSOLIDATED_REPORT"
        fi
    done
    
    # Adicionar conclusão geral
    cat >> "$CONSOLIDATED_REPORT" << 'EOF'

## Conclusão Geral dos Experimentos

### Principais Descobertas

1. **Overhead de Monitoramento**: O profiler desenvolvido tem impacto mínimo no sistema monitorado, sendo viável para uso em produção.

2. **Isolamento via Namespaces**: Os namespaces do Linux fornecem isolamento efetivo com overhead desprezível, sendo a base fundamental para containerização.

3. **Limitação de CPU**: Os cgroups v2 permitem controle preciso de CPU com throttling efetivo, essencial para ambientes multi-tenant.

4. **Limitação de Memória**: Limites de memória são aplicados rigidamente pelo kernel, com possível invocação do OOM killer quando excedidos.

5. **Limitação de I/O**: O controle de I/O é efetivo para dispositivos de bloco reais, mas pode ter limitações em dispositivos virtuais.

### Aplicações Práticas

Estes experimentos demonstram as capacidades fundamentais que permitem a containerização moderna:

- **Docker/Podman**: Utilizam namespaces para isolamento e cgroups para limitação de recursos
- **Kubernetes**: Gerencia recursos de containers através de cgroups
- **Systemd**: Utiliza cgroups para controlar recursos de serviços

### Recomendações para Produção

1. **Monitoramento**: Implementar coleta de métricas com intervalos adequados ao workload
2. **Isolamento**: Usar combinações apropriadas de namespaces baseadas nas necessidades de segurança
3. **Limites de Recursos**: Configurar limites com margem de segurança (10-20% acima do esperado)
4. **Alertas**: Monitorar estatísticas de throttling e memory.failcnt para ajustar limites

### Trabalhos Futuros

- Avaliar overhead de diferentes combinações de namespaces
- Testar limitação de CPU em workloads multi-threaded
- Comparar performance com outras ferramentas (docker stats, systemd-cgtop)
- Implementar dashboard em tempo real para visualização

---

**Autores**: [Adicionar nomes dos integrantes]
**Disciplina**: Sistemas Operacionais / Containers e Recursos
**Data**: $(date +"%B %Y")

EOF
    
    echo -e "${GREEN}✓ Relatório consolidado gerado!${NC}"
    echo -e "${BLUE}Localização: $CONSOLIDATED_REPORT${NC}"
    echo ""
}

# Função para verificar dependências
check_dependencies() {
    echo -e "${YELLOW}Verificando dependências...${NC}"
    
    local missing_deps=()
    
    # Verificar ferramentas necessárias
    for cmd in stress bc gcc make; do
        if ! command -v $cmd &> /dev/null; then
            missing_deps+=($cmd)
        fi
    done
    
    if [ ${#missing_deps[@]} -gt 0 ]; then
        echo -e "${YELLOW}AVISO: Dependências faltando: ${missing_deps[@]}${NC}"
        echo "Instale com: sudo apt-get install ${missing_deps[@]}"
        echo ""
        read -p "Continuar mesmo assim? (s/N): " -n 1 -r
        echo ""
        if [[ ! $REPLY =~ ^[Ss]$ ]]; then
            exit 1
        fi
    else
        echo -e "${GREEN}✓ Todas as dependências estão instaladas${NC}"
    fi
    echo ""
}

# Função para listar resultados existentes
list_results() {
    echo -e "${BLUE}Resultados existentes:${NC}"
    echo ""
    
    if [ -d "$PROJECT_ROOT/experimentos" ]; then
        for exp_dir in "$PROJECT_ROOT/experimentos"/exp*; do
            if [ -d "$exp_dir" ]; then
                local exp_name=$(basename "$exp_dir")
                local report="$exp_dir/RELATORIO.md"
                
                if [ -f "$report" ]; then
                    echo -e "  ${GREEN}✓${NC} $exp_name"
                else
                    echo -e "  ${YELLOW}○${NC} $exp_name (incompleto)"
                fi
            fi
        done
        echo ""
    else
        echo -e "  ${YELLOW}Nenhum resultado encontrado${NC}"
        echo ""
    fi
}

# Loop principal
check_dependencies

while true; do
    clear
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║  EXPERIMENTOS - CONTAINERS E RECURSOS                      ║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    
    list_results
    show_menu
    
    read -r option
    
    case $option in
        1)
            run_experiment 1
            ;;
        2)
            run_experiment 2
            ;;
        3)
            run_experiment 3
            ;;
        4)
            run_experiment 4
            ;;
        5)
            run_experiment 5
            ;;
        6)
            run_all
            ;;
        7)
            generate_consolidated_report
            read -p "Pressione ENTER para continuar..."
            ;;
        0)
            echo ""
            echo -e "${GREEN}Até logo!${NC}"
            echo ""
            exit 0
            ;;
        *)
            echo -e "${RED}Opção inválida!${NC}"
            sleep 2
            ;;
    esac
done
