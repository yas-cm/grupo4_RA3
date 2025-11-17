# Arquitetura do Resource Monitor

## Visão Geral

O Resource Monitor é um sistema de monitoramento e análise de recursos para processos Linux, desenvolvido em C. O projeto é estruturado em três componentes principais que trabalham de forma integrada:

1. **Resource Profiler** - Monitoramento de recursos
2. **Namespace Analyzer** - Análise de isolamento
3. **Control Group Manager** - Gerenciamento de cgroups

## Estrutura do Projeto

```
resource-monitor/
├── include/                    # Arquivos de cabeçalho
│   ├── monitor.h               # Interface do Resource Profiler
│   ├── namespace.h             # Interface do Namespace Analyzer
│   └── cgroup_manager.h        # Interface do Control Group Manager
├── src/                        # Código-fonte
│   ├── main.c                  # Ponto de entrada e CLI
│   ├── cpu_monitor.c           # Monitoramento de CPU
│   ├── memory_monitor.c        # Monitoramento de memória
│   ├── io_monitor.c            # Monitoramento de I/O
│   ├── net_monitor.c           # Monitoramento de rede
│   ├── namespace_analyzer.c    # Análise de namespaces
│   └── cgroup_manager.c        # Gerenciamento de cgroups
├── obj/                        # Arquivos objeto compilados
├── docs/                       # Documentação
├── scripts/                    # Scripts de testes
├── tests/                      # Resultados dos testes
├── Makefile                    # Sistema de build
└── README.md                   # Documentação principal
```

## Componentes

### 1. Resource Profiler

**Responsabilidade**: Monitorar em tempo real o consumo de recursos de processos Linux.

**Arquivos**:
- `src/cpu_monitor.c` - Coleta de métricas de CPU
- `src/memory_monitor.c` - Coleta de métricas de memória
- `src/io_monitor.c` - Coleta de métricas de I/O
- `src/net_monitor.c` - Coleta de métricas de rede
- `include/monitor.h` - Interface pública

**Funcionalidades**:
- Monitoramento de múltiplos processos simultaneamente
- Coleta de métricas:
  - **CPU**: Uso percentual (com EMA), threads
  - **Memória**: RSS e Swap em MB
  - **I/O**: Taxas de leitura/escrita em disco
  - **Rede**: Taxas RX/TX
  - **Avançadas**: Context switches, page faults, syscalls
- Modos de visualização: padrão e detalhado (verbose)
- Exportação para CSV
- Intervalo configurável de atualização

**Implementação**:
- Leitura de `/proc/[pid]/stat`, `/proc/[pid]/status`, `/proc/[pid]/io`
- Cálculo de médias móveis exponenciais (EMA) para suavização
- Formatação de saída para terminal e CSV

### 2. Namespace Analyzer

**Responsabilidade**: Inspecionar e analisar o nível de isolamento de processos através de namespaces Linux.

**Arquivos**:
- `src/namespace_analyzer.c` - Implementação da análise
- `include/namespace.h` - Interface pública

**Funcionalidades**:
- Listagem dos 7 tipos de namespaces de um processo:
  - PID (Process ID)
  - Net (Network)
  - Mnt (Mount)
  - IPC (Inter-Process Communication)
  - UTS (Hostname)
  - User (User ID)
  - Cgroup (Control Group)
- Comparação de isolamento entre dois processos
- Busca de processos que compartilham um namespace específico
- Relatório geral do sistema mostrando todos os namespaces ativos

**Implementação**:
- Leitura de `/proc/[pid]/ns/*` para identificar namespaces
- Comparação de inodes para determinar isolamento
- Varredura de `/proc` para busca global
- Agregação de dados para relatórios

### 3. Control Group Manager

**Responsabilidade**: Manipular e analisar cgroups v2 para limitação de recursos.

**Arquivos**:
- `src/cgroup_manager.c` - Implementação do gerenciamento
- `include/cgroup_manager.h` - Interface pública

**Funcionalidades**:
- Criação de novos cgroups na hierarquia unificada
- Aplicação de limites:
  - **CPU**: Percentual de uso
  - **Memória**: Limite em MB
- Movimentação de processos entre cgroups
- Geração de relatórios detalhados:
  - Uso atual vs limites configurados
  - Estatísticas de throttling (CPU)
  - Contadores de falhas (memória)

**Implementação**:
- Interface com `/sys/fs/cgroup/` (cgroups v2)
- Manipulação de arquivos de controle:
  - `cgroup.procs` - Processos no grupo
  - `cpu.max` - Limite de CPU
  - `memory.max` - Limite de memória
  - `cpu.stat` - Estatísticas de CPU
  - `memory.stat` - Estatísticas de memória
- Parsing de arquivos de estatísticas

## Fluxo de Dados

### Monitoramento de Recursos

```
Processo Alvo
     ↓
 /proc/[pid]/
     ↓
[Coletores de Métricas]
     ├── CPU Monitor
     ├── Memory Monitor
     ├── I/O Monitor
     └── Net Monitor
     ↓
[Processamento]
     ├── Cálculo de Deltas
     ├── EMA (Suavização)
     └── Formatação
     ↓
[Saída]
     ├── Terminal (stdout)
     └── CSV (arquivo)
```

### Análise de Namespaces

```
Processo(s) Alvo
     ↓
 /proc/[pid]/ns/
     ↓
[Namespace Analyzer]
     ├── Leitura de Inodes
     ├── Comparação
     └── Busca Global
     ↓
[Relatório]
     └── Terminal (stdout)
```

### Gerenciamento de Cgroups

```
Comandos do Usuário
     ↓
[Cgroup Manager]
     ↓
/sys/fs/cgroup/
     ├── Criar/Deletar
     ├── Aplicar Limites
     ├── Mover Processos
     └── Ler Estatísticas
     ↓
[Kernel Linux]
     └── Enforce Limits
```

## Interface de Linha de Comando

O programa utiliza uma interface CLI flexível com as seguintes opções:

### Monitoramento
- `--pids <pid1,pid2,...>` - Processos a monitorar
- `--intervalo <segundos>` - Intervalo de atualização
- `--verbose` ou `-v` - Modo detalhado
- `--csv <arquivo>` - Exportar para CSV

### Namespaces
- `--list-ns <pid>` - Listar namespaces de um processo
- `--compare-ns <pid1,pid2>` - Comparar dois processos
- `--find-ns <pid> <tipo>` - Encontrar processos em um namespace
- `--report-ns` - Relatório geral do sistema

### Cgroups
- `--cg-create <nome>` - Criar cgroup
- `--cg-delete <nome>` - Deletar cgroup
- `--cg-set-cpu <nome> <percent>` - Limitar CPU
- `--cg-set-mem <nome> <MB>` - Limitar memória
- `--cg-move <pid> <cgroup>` - Mover processo
- `--cg-report <nome>` - Relatório do cgroup

## Requisitos de Sistema

### Sistema Operacional
- **Linux** com kernel 4.5+ (suporte a cgroups v2)
- Distribuições recomendadas:
  - Ubuntu 22.04+
  - Fedora 31+
  - Debian 11+

### Verificação de Cgroups v2
```bash
stat -fc %T /sys/fs/cgroup
# Deve retornar: cgroup2fs
```

### Privilégios
- **Monitoramento básico**: Usuário normal
- **Análise de namespaces**: `sudo` (para processos de outros usuários)
- **Gerenciamento de cgroups**: `sudo` (requerido)

## Compilação

O projeto utiliza um `Makefile` que automatiza o processo:

```makefile
# Variáveis principais
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
SRCDIR = src
OBJDIR = obj
TARGET = resource_monitor

# Compilação
$(TARGET): $(OBJS)
    $(CC) $(OBJS) -o $(TARGET)

# Limpeza
clean:
    rm -rf $(OBJDIR) $(TARGET)
```

### Comandos
```bash
make        # Compilar
make clean  # Limpar
```

## Dependências

- **Compilador**: GCC 11.4.0+
- **Bibliotecas**: Apenas `libc` padrão (sem dependências externas)
- **Ferramentas de teste**: `stress`, `bc` (para testes)

## Qualidade de Código

### Padrões Seguidos
- Compilação sem warnings (`-Wall -Wextra`)
- Código comentado e bem estruturado
- Tratamento de erros consistente
- Sem memory leaks (validado com `valgrind`)

### Validação
```bash
# Compilar com flags extras
gcc -Wall -Wextra -Werror -g -fsanitize=address

# Verificar memory leaks
valgrind --leak-check=full ./resource_monitor --pids 1234
```

## Limitações Conhecidas

1. **Cgroups v1**: Não suportado. Apenas cgroups v2 (unified hierarchy)
2. **WSL2**: Limitações nos controladores de memória e I/O
3. **Dispositivos especiais**: Alguns dispositivos (tmpfs) não suportam limite de I/O
4. **Processos efêmeros**: Processos de curta duração podem não ser capturados
5. **Precisão de tempo**: Resolução de 1 segundo no intervalo de coleta

## Extensibilidade

### Adicionar Nova Métrica

1. Adicionar função em `src/*_monitor.c`:
```c
double get_new_metric(int pid) {
    // Ler de /proc/[pid]/...
    // Processar
    // Retornar valor
}
```

2. Adicionar ao header `include/monitor.h`

3. Integrar em `src/main.c` no loop de monitoramento

### Adicionar Novo Tipo de Namespace

1. Adicionar enum em `include/namespace.h`:
```c
typedef enum {
    NS_PID, NS_NET, NS_MNT, NS_IPC, 
    NS_UTS, NS_USER, NS_CGROUP,
    NS_NEW_TYPE  // Novo tipo
} NamespaceType;
```

2. Atualizar funções em `src/namespace_analyzer.c`

## Segurança

### Considerações
- Leitura de `/proc` requer cuidado com race conditions
- Validação de entrada do usuário (PIDs, nomes de cgroups)
- Verificação de permissões antes de operações privilegiadas
- Sanitização de caminhos de arquivo

### Operações Privilegiadas
Todas as operações que modificam o sistema requerem root:
- Criação/deleção de cgroups
- Movimentação de processos entre cgroups
- Aplicação de limites de recursos

## Performance

### Otimizações Implementadas
- Cálculo de deltas incremental (não recalcula tudo a cada iteração)
- Média móvel exponencial (EMA) para suavização sem histórico grande
- Leitura seletiva de `/proc` (apenas campos necessários)
- Buffers pré-alocados para strings

### Benchmarks
- Overhead de monitoramento: < 0.01% de CPU
- Memória utilizada: ~2MB por processo monitorado
- Latência de coleta: ~10ms por processo

## Testes

### Testes Implementados
O projeto inclui 5 testes automatizados:

1. **Overhead de Monitoramento** - Valida baixo impacto no sistema
2. **Isolamento via Namespaces** - Verifica efetividade dos 7 tipos
3. **Throttling de CPU** - Testa precisão de limites (25%, 50%, 100%, 200%)
4. **Limitação de Memória** - Valida comportamento ao atingir limite
5. **Limitação de I/O** - Avalia controle de throughput

### Execução
```bash
cd scripts
./run_experiments.sh  # Menu interativo
```

### Documentação dos Testes
- `tests/README.md` - Documentação completa
- `tests/RELATORIO_CONSOLIDADO.md` - Resultados (gerado após execução)

## Referências

- [Linux Programmer's Manual - proc(5)](https://man7.org/linux/man-pages/man5/proc.5.html)
- [cgroups(7) - Control Groups](https://man7.org/linux/man-pages/man7/cgroups.7.html)
- [namespaces(7) - Linux Namespaces](https://man7.org/linux/man-pages/man7/namespaces.7.html)
- [Kernel Documentation - cgroup-v2](https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html)

---

**Última atualização**: Novembro de 2025
