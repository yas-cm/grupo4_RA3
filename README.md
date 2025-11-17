# Resource Monitor

> Sistema de monitoramento e análise de recursos para processos Linux, desenvolvido em C.

Este projeto fornece uma ferramenta de linha de comando para monitorar em tempo real o consumo de recursos de processos Linux, analisar seu nível de isolamento através de namespaces e gerenciar a alocação de recursos via control groups (cgroups). Os dados podem ser visualizados diretamente no terminal ou exportados para um arquivo CSV.

## Status do Projeto

- **Componente 1: Resource Profiler - Concluído**
- **Componente 2: Namespace Analyzer - Concluído**
- **Componente 3: Control Group Manager - Concluído**

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

### Control Group Manager
O `Control Group Manager` permite a manipulação e análise de cgroups v2 para limitação de recursos:

- **Criação de Grupos:** Crie novos cgroups na hierarquia unificada do sistema.
- **Aplicação de Limites:** Defina limites de uso de CPU (percentual) e Memória (em MB).
- **Gerenciamento de Processos:** Mova processos para dentro e fora de cgroups.
- **Relatórios Detalhados:** Gere relatórios que mostram o uso atual de recursos versus os limites configurados, incluindo estatísticas de *throttling* (quando o processo foi "freado" pelo kernel).

## Requisitos

- **Sistema Operacional:** Linux com **cgroups v2** (padrão em Ubuntu 22.04+, Fedora 31+, etc.)
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

#### 2. Comparar o Isolamento de Dois Processos
Compare o seu terminal com um processo do sistema (`systemd-logind`):
```bash
# Descubra o PID do seu terminal
echo $$
# Supondo que o PID seja 35210 e o do systemd-logind seja 989
sudo ./resource_monitor --compare-ns 35210,989
```

#### 3. Encontrar Processos em um Namespace
Encontre todos os processos que compartilham o namespace de rede do processo init (PID 1):
```bash
sudo ./resource_monitor --find-ns 1 net
```

#### 4. Gerar Relatório Completo de Namespaces
Obtenha um sumário de todos os namespaces do sistema (ótimo para ver containers!):
```bash
sudo ./resource_monitor --report-ns
```

### Exemplos de Uso - Gerenciamento de Cgroups

*(Nota: Todas as operações de gerenciamento de cgroup exigem privilégios de root para modificar os arquivos de sistema em `/sys/fs/cgroup`. Use `sudo` para todos os comandos a seguir.)*

Este fluxo de trabalho completo demonstra como criar um cgroup, aplicar limites de CPU, mover um processo para dentro dele e verificar se os limites estão sendo aplicados.

#### 1. Crie um novo Cgroup
Crie um grupo chamado `meu-app` para os controladores de CPU, Memória e I/O:```bash
sudo ./resource_monitor --cg-create meu-app
```

#### 2. Aplique Limites de Recursos
Defina um limite de 25% de uso de um núcleo de CPU para o grupo:
```bash
# Limita a 25% de CPU
sudo ./resource_monitor --cg-set-cpu meu-app 25
```

#### 3. Inicie um Processo e Mova-o para o Cgroup
Vamos usar a ferramenta `stress` para gerar carga. Primeiro, instale-a (`sudo apt install stress`).

```bash
# Inicie o stress em background para usar 1 núcleo de CPU
stress --cpu 1 &

# Descubra o PID do processo trabalhador do stress (geralmente o maior PID)
pgrep stress

# Supondo que o PID seja 18072, mova-o para o cgroup
sudo ./resource_monitor --cg-move 18072 meu-app
```

#### 4. Verifique o Limite em Tempo Real
Em outro terminal, use o próprio `resource_monitor` para observar o processo. Você verá que o uso de CPU dele estará cravado em ~25%.
```bash
./resource_monitor --pids 18072 --intervalo 1
```
Saída esperada:
```
PID     NOME                 CPU %     MEM (MB) ...
---------------------------------------------------
18072   stress               24.92%    0.4MB    ...
```

#### 5. Gere um Relatório Final
Após deixar o processo rodar por alguns segundos, gere um relatório para ver as estatísticas acumuladas, incluindo quantas vezes o kernel precisou "frear" (throttle) o processo.
```bash
sudo ./resource_monitor --cg-report meu-app
```
Saída esperada:
```
--- Relatório do Cgroup v2: meu-app ---

[CPU]
    - Uso Total: 32.802 segundos
    - Ocorrências de Throttling: 1311
    - Tempo Total em Throttling: 98.241 segundos
  Limite Configurado: 25000 100000
...
```

#### 6. Limpeza
Não se esqueça de parar o processo de teste e remover o cgroup.
```bash
# Para o processo
sudo killall stress

# Remove o diretório do cgroup
sudo rmdir /sys/fs/cgroup/meu-app

# Remova o cgroup (só funciona se estiver vazio)
sudo ./resource_monitor --cg-delete meu-app
```

## Arquitetura

Para informações detalhadas sobre a arquitetura do projeto, estrutura de componentes, fluxo de dados e design interno, consulte:

**[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)**

## Qualidade de Código
- Compilar sem warnings (-Wall -Wextra)
- Código comentado e bem estruturado
- Makefile funcional
- README com instruções de compilação e uso
- **Sem memory leaks (validado com valgrind)**

## Experimentos

O projeto inclui 5 experimentos que validam o monitoramento, isolamento e limitação de recursos:

1. **Overhead de Monitoramento** - Impacto do profiler no sistema
2. **Isolamento via Namespaces** - Efetividade dos 7 tipos de namespaces  
3. **Throttling de CPU** - Precisão de limites via cgroups
4. **Limitação de Memória** - Comportamento ao atingir limite
5. **Limitação de I/O** - Controle de throughput de disco

### Como Executar

```bash
cd scripts
./run_experiments.sh  # Menu interativo (requer sudo)
```

### Documentação dos Experimentos

- **Documentação Completa**: `experimentos/README.md`
- **Resultados**: Gerados automaticamente em `experimentos/` após execução

## Documentação Adicional

- **[Arquitetura do Projeto](docs/ARCHITECTURE.md)** - Design interno e estrutura de componentes
- **[Experimentos](experimentos/README.md)** - Guia completo dos 5 experimentos obrigatórios

