# Experimentos Obrigatórios - RA3

Este diretório contém os scripts e documentação dos 5 experimentos obrigatórios do projeto de Containers e Recursos.

## 📋 Visão Geral dos Experimentos

### Experimento 1: Overhead de Monitoramento
**Objetivo**: Medir o impacto do próprio profiler no sistema

**O que é testado**:
- Execução de workload com e sem monitoramento
- Impacto do intervalo de coleta nas métricas
- Overhead de CPU introduzido pelo profiler

**Métricas coletadas**:
- Tempo de execução total
- CPU overhead (%)
- Latência de sampling

### Experimento 2: Isolamento via Namespaces
**Objetivo**: Validar efetividade do isolamento

**O que é testado**:
- Criação de diferentes tipos de namespaces (PID, NET, UTS, etc.)
- Verificação de visibilidade de recursos
- Overhead de criação de namespaces

**Métricas coletadas**:
- Tempo de criação de cada namespace (µs)
- Isolamento efetivo (quantos processos/recursos visíveis)
- Número de processos por namespace no sistema

### Experimento 3: Throttling de CPU
**Objetivo**: Avaliar precisão de limitação de CPU via cgroups

**O que é testado**:
- Workload CPU-intensive com diferentes limites (25%, 50%, 100%, 200%)
- Precisão do throttling do kernel
- Estatísticas de "freamento" do processo

**Métricas coletadas**:
- CPU% medido vs limite configurado
- Desvio percentual
- Ocorrências de throttling
- Tempo total em throttling

### Experimento 4: Limitação de Memória
**Objetivo**: Testar comportamento ao atingir limite de memória

**O que é testado**:
- Alocação incremental de memória
- Comportamento ao atingir o limite (malloc() fail ou OOM killer)
- Estatísticas de falhas de alocação

**Métricas coletadas**:
- Quantidade máxima alocada
- Número de falhas (memory.failcnt)
- Logs do kernel (OOM events)

### Experimento 5: Limitação de I/O
**Objetivo**: Avaliar precisão de limitação de I/O

**O que é testado**:
- Workload I/O-intensive (leitura e escrita sequencial)
- Diferentes limites de throughput (10, 50, 100 MB/s)
- Impacto na latência

**Métricas coletadas**:
- Throughput de leitura/escrita medido vs limite
- Latência de I/O
- Tempo total de execução

## 🚀 Como Executar

### Método 1: Script Interativo (Recomendado)

Execute o script mestre que apresenta um menu interativo:

```bash
cd scripts
./run_experiments.sh
```

O script irá:
- Verificar dependências
- Mostrar menu com opções
- Permitir execução individual ou de todos os experimentos
- Gerar relatório consolidado automaticamente

### Método 2: Execução Individual

Execute cada experimento separadamente:

```bash
cd scripts

# Experimento 1 (não precisa sudo)
./experimento1_overhead.sh

# Experimentos 2-5 (precisam sudo)
sudo ./experimento2_namespaces.sh
sudo ./experimento3_cpu_throttling.sh
sudo ./experimento4_memory_limit.sh
sudo ./experimento5_io_limit.sh
```

## 📦 Dependências

Certifique-se de ter instalado:

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    stress \
    bc \
    poppler-utils
```

## 📊 Resultados

Após a execução, os resultados estarão organizados em:

```
experimentos/
├── exp1_overhead/
│   ├── RELATORIO.md
│   ├── overhead_results.csv
│   └── *.csv (dados de monitoramento)
├── exp2_namespaces/
│   ├── RELATORIO.md
│   ├── namespace_overhead.csv
│   └── *.txt (análises de isolamento)
├── exp3_cpu_throttling/
│   ├── RELATORIO.md
│   ├── cpu_throttling_results.csv
│   └── cgroup_report_*.txt
├── exp4_memory_limit/
│   ├── RELATORIO.md
│   ├── memory_results.txt
│   └── memory_usage.csv
├── exp5_io_limit/
│   ├── RELATORIO.md
│   └── io_results.csv
└── RELATORIO_CONSOLIDADO.md
```

Cada diretório contém:
- **RELATORIO.md**: Análise detalhada com conclusões
- **\*.csv**: Dados brutos para análise posterior
- **\*.txt**: Outputs e logs dos testes

## 📈 Visualização de Resultados

Para visualizar os relatórios:

```bash
# Relatório consolidado de todos os experimentos
cat experimentos/RELATORIO_CONSOLIDADO.md

# Relatório individual
cat experimentos/exp1_overhead/RELATORIO.md
```

Para plotar gráficos (usando o script de visualização):

```bash
cd scripts
./visualize.sh
```

## ⚠️ Notas Importantes

### Privilégios Root
Os experimentos 2-5 **precisam de privilégios root** porque:
- Criam e manipulam cgroups em `/sys/fs/cgroup`
- Criam novos namespaces
- Movem processos entre cgroups
- Leem informações de processos do sistema

### Cgroups v2
Os scripts foram desenvolvidos para **cgroups v2** (unified hierarchy).

Verifique se seu sistema usa cgroups v2:
```bash
stat -fc %T /sys/fs/cgroup
# Deve retornar: cgroup2fs
```

Se retornar `tmpfs`, seu sistema ainda usa cgroups v1. Ubuntu 22.04+ usa v2 por padrão.

### Impacto no Sistema
Os experimentos são **seguros** mas podem:
- Usar 100% de CPU temporariamente (Exp 1 e 3)
- Alocar até 200MB de RAM (Exp 4)
- Gerar I/O intenso no disco (Exp 5)

**Recomendação**: Execute em VM ou sistema de testes, não em produção.

### Duração
Tempo estimado para executar todos os experimentos: **5-10 minutos**

## 🔧 Troubleshooting

### "comando 'stress' não encontrado"
```bash
sudo apt-get install stress
```

### "Sistema não parece usar cgroups v2"
Seu sistema usa cgroups v1. Considere:
1. Atualizar para Ubuntu 22.04+
2. Habilitar cgroups v2 manualmente
3. Usar uma VM com Ubuntu 22.04+

### "Não foi possível aplicar limite de I/O"
Alguns dispositivos (tmpfs, certos SSDs) não suportam limitação via `io.max`.
Isso é esperado e o script irá avisar.

### "Permission denied" mesmo com sudo
Certifique-se de:
1. Estar realmente executando com sudo
2. SELinux/AppArmor não está bloqueando
3. Ter permissões de escrita em `/sys/fs/cgroup`

## 📚 Documentação Adicional

- **Arquitetura do projeto**: `../docs/ARCHITECTURE.md`
- **README principal**: `../README.md`
- **Código-fonte**: `../src/`

## 🎯 Critérios de Avaliação Atendidos

- ✅ **Experimento 1**: Overhead de monitoramento medido e documentado
- ✅ **Experimento 2**: Isolamento validado com todos os tipos de namespace
- ✅ **Experimento 3**: Throttling testado com múltiplos limites
- ✅ **Experimento 4**: Comportamento de limite de memória documentado
- ✅ **Experimento 5**: Limitação de I/O avaliada

Todas as métricas obrigatórias são coletadas e reportadas em formato legível.

## 👥 Autores

[Adicionar nomes dos integrantes do grupo]

## 📅 Data de Entrega

[Adicionar data]

---

**Dica**: Para uma experiência melhor, execute o script interativo:
```bash
./scripts/run_experiments.sh
```
