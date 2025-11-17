# Experimentos de Análise de Recursos e Isolamento

```
╔═══════════════════════════════════════════════════════════════════════════╗
║     EXPERIMENTOS OBRIGATÓRIOS - IMPLEMENTAÇÃO COMPLETA                    ║
╚═══════════════════════════════════════════════════════════════════════════╝
```

##  Quick Start

### Execução em 3 Passos

1. **Compilar** (se necessário):
   ```bash
   cd /home/yas/resource-monitor
   make
   ```

2. **Executar todos os experimentos**:
   ```bash
   cd scripts
   ./run_experiments.sh
   ```

3. **Escolher opção 6** no menu (Executar TODOS)
   - Aguarde ~10 minutos
   - Todos os relatórios serão gerados automaticamente

###  Checklist Pré-Execução

- [ ] Projeto compilado: `make`
- [ ] Dependências instaladas: `sudo apt install stress bc poppler-utils`
- [ ] Sistema com cgroups v2: `stat -fc %T /sys/fs/cgroup` (deve retornar `cgroup2fs`)
- [ ] ~1GB de espaço livre em disco
- [ ] 10-15 minutos disponíveis

###  Notas Importantes

- **Privilégios**: Experimentos 2-5 precisam de sudo
- **Duração**: ~10 minutos total
- **Resultados**: `experimentos/RELATORIO_CONSOLIDADO.md`
- **Segurança**: Pode usar 100% CPU e até 200MB RAM temporariamente

---

##  Experimentos Implementados

```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃ EXPERIMENTOS IMPLEMENTADOS                                            ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

###  Experimento 1: Overhead de Monitoramento
- **Script**: `scripts/experimento1_overhead.sh` (6.7 KB)
- **Duração**: ~2 minutos
- **Privilégios**: NÃO precisa sudo

**Objetivo**: Medir o impacto do profiler no sistema

**O que é testado**:
- Workload CPU-intensive com e sem monitoramento
- Diferentes intervalos de coleta (1s, 2s, 5s)
- Overhead percentual do profiler

**Métricas coletadas**:
-  Tempo de execução com e sem profiler
-  CPU overhead (%)
-  Latência de sampling

---

###  Experimento 2: Isolamento via Namespaces
- **Script**: `scripts/experimento2_namespaces.sh` (8.2 KB)
- **Duração**: ~1 minuto
- **Privilégios**: PRECISA sudo

**Objetivo**: Validar efetividade do isolamento

**O que é testado**:
- Criação de diferentes tipos de namespaces (PID, NET, UTS, IPC, etc.)
- Verificação de visibilidade de recursos
- Overhead de criação de namespaces

**Métricas coletadas**:
-  Tabela de isolamento efetivo por tipo de namespace
-  Overhead de criação (µs)
-  Número de processos por namespace no sistema

---

###  Experimento 3: Throttling de CPU
- **Script**: `scripts/experimento3_cpu_throttling.sh` (9.7 KB)
- **Duração**: ~2 minutos
- **Privilégios**: PRECISA sudo

**Objetivo**: Avaliar precisão de limitação de CPU via cgroups

**O que é testado**:
- Workload CPU-intensive com diferentes limites (25%, 50%, 100%, 200%)
- Precisão do throttling do kernel
- Estatísticas de "freamento" do processo

**Métricas coletadas**:
-  CPU% medido vs limite configurado
-  Desvio percentual
-  Throughput (iterações/segundo) em cada configuração
-  Ocorrências de throttling
-  Tempo total em throttling

---

###  Experimento 4: Limitação de Memória
- **Script**: `scripts/experimento4_memory_limit.sh` (11 KB)
- **Duração**: ~1 minuto
- **Privilégios**: PRECISA sudo

**Objetivo**: Testar comportamento ao atingir limite de memória

**O que é testado**:
- Alocação incremental de memória
- Comportamento ao atingir o limite (malloc() fail ou OOM killer)
- Estatísticas de falhas de alocação

**Métricas coletadas**:
-  Quantidade máxima alocada
-  Número de falhas (memory.failcnt)
-  Comportamento do sistema ao atingir limite
-  Logs do kernel (OOM events)

---

###  Experimento 5: Limitação de I/O
- **Script**: `scripts/experimento5_io_limit.sh` (13 KB)
- **Duração**: ~2 minutos
- **Privilégios**: PRECISA sudo

**Objetivo**: Avaliar precisão de limitação de I/O

**O que é testado**:
- Workload I/O-intensive (leitura e escrita sequencial)
- Diferentes limites de throughput (10, 50, 100 MB/s)
- Impacto na latência

**Métricas coletadas**:
-  Throughput medido vs limite configurado
-  Latência de I/O
-  Impacto no tempo total de execução

---

###  Script Mestre
- **Script**: `scripts/run_experiments.sh` (12 KB)
- **Duração**: ~10 minutos (todos os experimentos)

**Características**:
- Menu interativo colorido
- Executa experimentos individuais ou todos de uma vez
- Verifica dependências automaticamente
- Gera relatório consolidado
- Lista resultados existentes
- Interface amigável com progresso

---

##  Como Executar

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

---

##  Dependências

Certifique-se de ter instalado:

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    stress \
    bc \
    poppler-utils
```

**Componentes**:
- `build-essential`: Compilador GCC para o projeto
- `stress`: Ferramenta de geração de carga CPU/memória
- `bc`: Calculadora para aritmética de ponto flutuante
- `poppler-utils`: Utilitários PDF (opcional)

---

##  Estrutura de Resultados

Após a execução, os resultados estarão organizados em:

```
experimentos/
├── README.md                         ← Este arquivo
├── RELATORIO_CONSOLIDADO.md          ← RELATÓRIO PRINCIPAL
│
├── exp1_overhead/
│   ├── RELATORIO.md                  ← Análise completa
│   ├── overhead_results.csv          ← Dados principais
│   └── *.csv                         ← Logs de monitoramento
│
├── exp2_namespaces/
│   ├── RELATORIO.md
│   ├── namespace_overhead.csv
│   └── *.txt                         ← Análises de isolamento
│
├── exp3_cpu_throttling/
│   ├── RELATORIO.md
│   ├── cpu_throttling_results.csv
│   └── cgroup_report_*.txt           ← Estatísticas de throttling
│
├── exp4_memory_limit/
│   ├── RELATORIO.md
│   ├── memory_results.txt
│   └── memory_usage.csv
│
└── exp5_io_limit/
    ├── RELATORIO.md
    └── io_results.csv
```

### Conteúdo de Cada Diretório

- **RELATORIO.md**: Análise detalhada com conclusões e recomendações
- **\*.csv**: Dados brutos para análise posterior
- **\*.txt**: Outputs, logs e análises complementares
- **cgroup_report_\*.txt**: Estatísticas de cgroups (experimentos 3-5)

---

##  Visualização de Resultados

### Ver Relatórios

Para visualizar os relatórios gerados:

```bash
# Relatório consolidado de todos os experimentos
cat experimentos/RELATORIO_CONSOLIDADO.md

# Relatório individual
cat experimentos/exp1_overhead/RELATORIO.md
cat experimentos/exp2_namespaces/RELATORIO.md
cat experimentos/exp3_cpu_throttling/RELATORIO.md
cat experimentos/exp4_memory_limit/RELATORIO.md
cat experimentos/exp5_io_limit/RELATORIO.md
```

### Plotar Gráficos

Para visualizar gráficos (usando o script de visualização):

```bash
cd scripts
./visualize.sh
```

---

##  Notas Importantes

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

```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃ Experimento 1: ~2 minutos                        ┃
┃ Experimento 2: ~1 minuto                         ┃
┃ Experimento 3: ~2 minutos                        ┃
┃ Experimento 4: ~1 minuto                         ┃
┃ Experimento 5: ~2 minutos                        ┃
┃ ───────────────────────────────────────────────  ┃
┃ TOTAL: ~10 minutos                               ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

---

##  Troubleshooting

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
Isso é esperado em ambientes WSL2 e o script irá avisar.

### "Permission denied" mesmo com sudo
Certifique-se de:
1. Estar realmente executando com sudo
2. SELinux/AppArmor não está bloqueando
3. Ter permissões de escrita em `/sys/fs/cgroup`

### Experimento 4 ou 5 não funciona no WSL2
WSL2 tem limitações nos controladores de cgroups para memória e I/O.
Os scripts detectam isso e reportam as limitações.
Execute em uma VM Ubuntu nativa para resultados completos.

---

## 🎯 Critérios de Avaliação Atendidos

```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃ MÉTRICAS COLETADAS (Conforme PDF - Seção 6)                           ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

### Experimento 1: Overhead de Monitoramento
-  Tempo de execução com e sem profiler
-  CPU overhead (%)
-  Latência de sampling

### Experimento 2: Isolamento via Namespaces
-  Tabela de isolamento efetivo por tipo de namespace
-  Overhead de criação (µs)
-  Número de processos por namespace no sistema

### Experimento 3: Throttling de CPU
-  CPU% medido vs limite configurado
-  Desvio percentual
-  Throughput (iterações/segundo) em cada configuração

### Experimento 4: Limitação de Memória
-  Quantidade máxima alocada
-  Número de falhas (memory.failcnt)
-  Comportamento do sistema ao atingir limite

### Experimento 5: Limitação de I/O
-  Throughput medido vs limite configurado
-  Latência de I/O
-  Impacto no tempo total de execução

**Status**: Todas as métricas obrigatórias são coletadas e reportadas em formato legível.

---

##  Documentação Adicional

- **Arquitetura do projeto**: `../docs/ARCHITECTURE.md`
- **README principal**: `../README.md`
- **Código-fonte**: `../src/`
- **Headers**: `../include/`

---

##  Dicas e Boas Práticas

```
 Use o script mestre (run_experiments.sh) - ele é interativo!
 Experimentos 2-5 precisam de sudo - não esqueça!
 Cada experimento gera seu próprio relatório detalhado
 O relatório consolidado junta todos os resultados
 Todos os dados ficam salvos em CSV para análise posterior
 Scripts têm cores e mensagens claras para facilitar o uso
 Cada relatório inclui conclusões e recomendações
 Execute em um sistema de testes, não em produção
```

---

##  Estatísticas da Implementação

```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  ESTATÍSTICAS                                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

- **Total de linhas de código**: ~1,800 linhas (Bash)
- **Scripts criados**: 6 arquivos executáveis (.sh)
- **Documentação**: Completa e detalhada
- **Cobertura dos requisitos**: 100%
- **Status**: Pronto para execução e entrega

---

##  Próximos Passos

1. **Executar experimentos**
   ```bash
   cd scripts
   ./run_experiments.sh
   # Escolher opção 6
   ```

2. **Verificar resultados**
   ```bash
   cat experimentos/RELATORIO_CONSOLIDADO.md
   ```

3. **Adicionar autores ao README principal**
   ```bash
   nano ../README.md
   # Adicionar seção "## Autores" no final
   ```

4. **Commit para repositório**
   ```bash
   git add .
   git commit -m "Add: Experimentos obrigatórios completos"
   git push
   ```