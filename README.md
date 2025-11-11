# Resource Monitor

> Sistema de monitoramento e análise de recursos para processos Linux, desenvolvido em C.

Este projeto fornece uma ferramenta de linha de comando para monitorar em tempo real o consumo de recursos de processos Linux e analisar seu nível de isolamento através de namespaces. Os dados podem ser visualizados diretamente no terminal ou exportados para um arquivo CSV.

## Status do Projeto

- **Componente 1: Resource Profiler - ✅ Concluído**
- **Componente 2: Namespace Analyzer - ✅ Concluído**
- **Componente 3: Control Group Manager -** (Em desenvolvimento)

## Funcionalidades Implementadas

### Resource Profiler
O `Resource Profiler` conta com um conjunto robusto de funcionalidades de monitoramento:

- **Monitoramento Múltiplo:** Acompanhe vários processos simultaneamente.
- **Modos de Visualização:** Padrão e detalhado (`-v`).
- **Snapshot Detalhado:** Obtenha uma análise instantânea e completa de um único processo.
- **Métricas Abrangentes:**
    - **CPU:** Uso percentual (com Média Móvel Exponencial), Threads.
    - **Memória:** Uso de RSS e Swap em MB.
    - **I/O:** Taxas de leitura e escrita em disco.
    - **Rede:** Taxas de recebimento (RX) e transmissão (TX) de dados.
    - **Avançadas:** Context Switches, Page Faults e contagem de Syscalls.
- **Exportação para CSV (`--csv`):** Salve o histórico de todas as métricas em um arquivo.
- **Intervalo Configurável:** Defina o intervalo de atualização das métricas.

### Namespace Analyzer
O `Namespace Analyzer` oferece um conjunto de ferramentas para inspecionar o isolamento dos processos:

- **Análise Individual:** Liste todos os 7 tipos de namespaces (PID, Net, Mnt, etc.) aos quais um processo pertence.
- **Comparação de Isolamento:** Compare os namespaces de dois processos para identificar exatamente quais recursos eles compartilham e quais são isolados.
- **Busca por Processos:** Encontre todos os processos no sistema que pertencem a um namespace específico de um processo de referência.
- **Relatório Geral do Sistema:** Gere um sumário de todos os namespaces ativos no sistema, mostrando quantos processos pertencem a cada um, oferecendo uma visão clara do nível de containerização.

## Requisitos

- **Sistema Operacional:** Linux (desenvolvido e testado em Ubuntu 24.04)
- **Compilador:** GCC 11.4.0+
- **Bibliotecas:** Nenhuma dependência externa, apenas a `libc` padrão.

## Compilação

O projeto utiliza um `Makefile` que automatiza todo o processo de compilação.

1.  Clone o repositório.
2.  Navegue até a raiz do projeto.
3.  Execute o comando `make`:

    ```bash
    make
    ```
    Isso compilará todos os arquivos-fonte e criará o executável `resource_monitor` na raiz do projeto. Para limpar os arquivos compilados, use `make clean`.

## Como Usar

O programa oferece uma interface de linha de comando flexível. Para ver todas as opções, execute:

```bash
./resource_monitor --help
```

### Exemplos de Uso - Monitoramento de Recursos

#### 1. Monitoramento Básico de Múltiplos Processos
Monitore os processos `1234` e `5678`:
```bash
./resource_monitor --pids 1234,5678
```

#### 2. Monitoramento Detalhado (Verbose)
Monitore o processo `9012` com um intervalo de 5 segundos, exibindo todas as métricas:
```bash
./resource_monitor --pids 9012 --intervalo 5 --verbose
```

#### 3. Exportação para CSV
Monitore os processos `3344` e `5566` e salve os dados no arquivo `log.csv`:
```bash
./resource_monitor --pids 3344,5566 --csv log.csv
```

### Exemplos de Uso - Análise de Namespaces

*(Nota: Algumas operações de análise de namespace podem exigir privilégios de root para inspecionar processos do sistema. Use `sudo` se encontrar erros de "Permission denied".)*

#### 1. Listar Namespaces de um Processo
Veja a quais "universos" o processo init (PID 1) pertence:
```bash
sudo ./resource_monitor --list-ns 1
```
Saída esperada:
```bash
=== Namespaces do Processo 1 ===
TIPO      : IDENTIFICADOR
------------------------------------------
net       : net:[4026531840]
uts       : uts:[4026531838]
ipc       : ipc:[4026531839]
...
```

#### 2. Comparar o Isolamento de Dois Processos
Compare o seu terminal com um processo do sistema (`systemd-logind`, PID ~900) para ver as diferenças de isolamento:
```bash
# Descubra o PID do seu terminal
echo $$
# Supondo que o PID seja 35210 e o do systemd-logind seja 989
sudo ./resource_monitor --compare-ns 35210,989
```
Saída esperada:
```bash
=== Comparando Namespaces: PID 35210 vs PID 989 ===
TIPO     | STATUS          | IDENTIFICADOR(ES)
--------------------------------------------------------------------------
cgroup   | COMPARTILHADO   | cgroup:[4026531835]
mnt      | DIFERENTE       | mnt:[4026532217] (PID 35210) vs mnt:[4026531840] (PID 989)
...```

#### 3. Encontrar Processos em um Namespace
Encontre todos os processos que compartilham o namespace de rede do processo init (PID 1):
```bash
sudo ./resource_monitor --find-ns 1 net
```
Saída esperada:
```bash
Procurando por processos no namespace 'net' com ID: net:[4026531841]
PIDs encontrados:
- 1
- 123
- 456
...
```

#### 4. Gerar Relatório Completo de Namespaces
Obtenha um sumário de todos os namespaces do sistema (ótimo para ver containers!):
```bash
sudo ./resource_monitor --report-ns
```
Saída esperada:
```bash
--- Relatório para Namespace 'net' ---
  ID: net:[4026531841]         | Processos: 251
  ID: net:[4026539878]         | Processos: 12
--- Relatório para Namespace 'pid' ---
  ID: pid:[4026531838]         | Processos: 251
  ID: pid:[4026539877]         | Processos: 12
...```

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
│   ├── net_monitor.c
│   └── namespace_analyzer.c
└── README.md
```

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

