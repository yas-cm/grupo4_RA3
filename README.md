# Resource Monitor

> Sistema de monitoramento e análise de recursos para processos Linux, desenvolvido em C.

Este projeto fornece uma ferramenta de linha de comando para monitorar em tempo real o consumo de CPU, memória, I/O e rede de um ou mais processos específicos no sistema operacional Linux. Os dados podem ser visualizados diretamente no terminal ou exportados para um arquivo CSV para análise posterior.

## Status do Projeto

- **Componente 1: Resource Profiler - ✅ Concluído**
- **Componente 2: Namespace Analyzer -** (Em desenvolvimento)
- **Componente 3: Control Group Manager -** (Em desenvolvimento)

## Funcionalidades Implementadas

O `Resource Profiler` já conta com um conjunto robusto de funcionalidades:

- **Monitoramento Múltiplo:** Acompanhe vários processos simultaneamente, fornecendo uma lista de PIDs.
- **Modos de Visualização:**
    - **Padrão:** Uma tabela limpa e concisa, ideal para uma visão geral rápida.
    - **Verbose (`-v`):** Exibição detalhada de todas as métricas para cada processo.
- **Snapshot Detalhado:** Obtenha uma análise instantânea e completa de um único processo.
- **Métricas Abrangentes:**
    - **CPU:** Uso percentual (calculado com Média Móvel Exponencial para estabilidade), número de Threads.
    - **Memória:** Uso de RSS e Swap em MB.
    - **I/O:** Taxas de leitura e escrita em disco (B/s, KB/s, MB/s).
    - **Rede:** Taxas de recebimento (RX) e transmissão (TX) de dados.
    - **Avançadas:** Context Switches (voluntários/não voluntários), Page Faults (minor/major) e contagem de Syscalls.
- **Exportação para CSV (`--csv`):** Salve o histórico de todas as métricas em um arquivo CSV para análise de dados e geração de gráficos.
- **Intervalo Configurável:** Defina o intervalo de atualização das métricas em segundos.
- **Tratamento de Erros:** Detecta e informa quando um processo não é encontrado ou é encerrado durante o monitoramento.

## Requisitos

- **Sistema Operacional:** Linux (desenvolvido e testado em Ubuntu 24.04)
- **Compilador:** GCC 11.4.0+
- **Bibliotecas:** Nenhuma dependência externa, apenas a `libc` padrão.

## Compilação

Para compilar o projeto, você pode usar o GCC diretamente. Recomenda-se o uso das flags `-Wall` e `-Wextra` para garantir a qualidade do código.

1.  Clone o repositório.
2.  Navegue até a raiz do projeto.
3.  Execute o comando de compilação:

    ```bash
    # Cria o diretório 'bin' se ele não existir
    mkdir -p bin

    # Compila todos os arquivos .c da pasta src e gera o executável em bin/
    gcc src/*.c -o bin/monitor -Wall -Wextra -lrt
    ```
    *(Nota: A flag `-lrt` é necessária para `clock_gettime`)*

## Como Usar

O programa oferece uma interface de linha de comando flexível. Para ver todas as opções, execute:

```bash
./bin/monitor --help
```

### Exemplos de Uso

#### 1. Monitoramento Básico de Múltiplos Processos
Monitore os processos com PIDs `1234` e `5678`, com o intervalo padrão de 2 segundos.

```bash
./bin/monitor --pids 1234,5678
```
Saída esperada:
```bash
===========  MONITORANDO 2 PROCESSOS (Intervalo: 2s)  ===========
PID     NOME             CPU%     MEMORIA    I/O LEITURA  I/O ESCRITA  REDE RX      REDE TX
----------------------------------------------------------------------------------------------------------
1234    firefox          12.3%    512.4MB    1.2KB/s      256B/s       4.5KB/s      1.1KB/s
5678    postgres         2.1%     128.9MB    8.7KB/s      45.2KB/s     0B/s         0B/s
```

#### 2.  Monitoramento Detalhado (Verbose)
Monitore o processo 9012 com um intervalo de 5 segundos, exibindo todas as métricas.
```bash
./bin/monitor --pids 9012 --intervalo 5 --verbose
```
Saída esperada:
```bash
===== MONITORAMENTO DETALhado (Intervalo: 5s, Iteracao: 1) =====

--------------------------------------------------
  PID: 9012                NOME: my-app
--------------------------------------------------
  CPU Usage: 25.40 %         Threads: 16
  Memory (RSS): 200.5 MB        Swap: 0.0 MB
  Context Switches (Vol/Non): 1024 / 128
  Page Faults (Minor/Major): 5120 / 2
  I/O Read/Write Rate: 1.5MB/s / 512KB/s
  Network RX/TX Rate: 0B/s / 0B/s
  Syscalls Read/Write: 8192 / 4096
```

#### 3. Exportação para CSV
Monitore os processos 3344 e 5566 e salve continuamente os dados no arquivo log.csv.
```bash
./bin/monitor --pids 3344,5566 --csv log.csv
```
O terminal exibirá a tabela padrão, e o arquivo log.csv será criado/atualizado com as métricas detalhadas a cada ciclo.

#### 4. Snapshot Único e Detalhado
Obtenha uma análise instantânea e completa do processo 7788.
```bash
./bin/monitor --pid 7788
```

#### 5. Snapshot com Métricas Avançadas do Kernel
Obtenha o snapshot detalhado e adicione informações como prioridade, valor nice e detalhes de conexões TCP.
```bash
./bin/monitor --pid 7788 --avancado
```

### Estrutura do projeto
```bash
resource-monitor/
├── include/
│   └── monitor.h
├── src/
│   ├── main.c
│   ├── cpu_monitor.c
│   ├── memory_monitor.c
│   ├── io_monitor.c
│   └── net_monitor.c
└── README.md
```

---
### 2. Namespace Analyzer  
- Listar todos os namespaces de um processo
- Encontrar processos em um namespace específico
- Comparar namespaces entre dois processos
- Gerar relatório de namespaces do sistema

### 3. Control Group Manager
- Ler métricas de CPU, Memory e BLKIO cgroups
- Criar cgroup experimental
- Mover processo para cgroup
- Aplicar limites de CPU e Memória
- Gerar relatório de utilização
- **+ Suporte a cgroup v2 (unified hierarchy)**

### 4. Qualidade de Código
- Compilar sem warnings (-Wall -Wextra)
- Código comentado e bem estruturado
- Makefile funcional
- README com instruções de compilação e uso
- **+ Sem memory leaks (validado com valgrind)**

