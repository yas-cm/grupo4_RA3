# Tests Directory

Este diretório contém testes funcionais para validar os limites de recursos (CPU, memória e I/O) usando cgroups v2.

## Testes Disponíveis

### 1. test_overhead - Teste de Overhead de Monitoramento

Mede o impacto (overhead) do monitoramento no desempenho do sistema, comparando a execução de um workload com e sem monitoramento ativo.

**Uso:**
```bash
./tests/test_overhead
```

**O que testa:**
- Executa workload CPU-intensive sem monitoramento (baseline)
- Executa o mesmo workload com monitoramento em diferentes intervalos (1000ms, 500ms, 100ms)
- Calcula overhead percentual para cada intervalo

**Saída:**
- Tempo de execução baseline
- Tempo com monitoramento em cada intervalo
- Overhead percentual de cada configuração

---

### 2. test_namespaces - Teste de Isolamento via Namespaces

Valida a efetividade do isolamento através da criação e verificação de diferentes tipos de namespaces.

**Uso:**
```bash
sudo ./tests/test_namespaces
```

**O que testa:**
- Criação de 5 tipos de namespaces (PID, NET, UTS, IPC, MNT)
- Verifica se o isolamento foi efetivo (IDs diferentes entre pai e filho)
- Mede tempo de criação de cada namespace

**Saída:**
- Status de isolamento para cada namespace
- Tempo de criação em microsegundos
- Taxa de sucesso dos testes

---

### 3. test_cpu - Teste de Limitação de CPU

Testa a precisão da limitação de CPU através de cgroups, executando um workload intensivo e medindo o uso real versus o limite configurado.

**Compilação:**
```bash
make tests
```

**Uso:**
```bash
sudo ./tests/test_cpu [cpu_limit_percent]
```

**Parâmetros:**
- `cpu_limit_percent`: Percentual de CPU permitido (1-400%). Padrão: 50%
  - 100 = 1 core completo
  - 200 = 2 cores completos
  - 50 = metade de 1 core

**Exemplo:**
```bash
sudo ./tests/test_cpu 50    # Limitar a 50% de 1 CPU
sudo ./tests/test_cpu 100   # Limitar a 1 CPU completo
sudo ./tests/test_cpu 200   # Limitar a 2 CPUs completos
```

**Saída:**
- Uso de CPU medido vs configurado
- Desvio percentual
- Número de eventos de throttling
- Tempo total de throttling

---

### 4. test_memory - Teste de Limitação de Memória

Testa o comportamento do sistema ao atingir limites de memória, verificando eventos de OOM (Out of Memory) e possível término do processo pelo OOM killer.

**Uso:**
```bash
sudo ./tests/test_memory [limit_mb] [max_alloc_mb]
```

**Parâmetros:**
- `limit_mb`: Limite de memória em MB. Padrão: 100 MB
- `max_alloc_mb`: Quantidade máxima a tentar alocar. Padrão: 150 MB

**Exemplo:**
```bash
sudo ./tests/test_memory 100 150   # Limite 100MB, tentar alocar 150MB
sudo ./tests/test_memory 50 100    # Limite 50MB, tentar alocar 100MB
```

**Saída:**
- Pico de uso de memória
- Eventos de OOM
- Número de processos terminados (OOM kills)

---

### 5. test_io - Teste de Limitação de I/O

Testa a precisão da limitação de taxa de I/O (leitura ou escrita) através de cgroups.

**Uso:**
```bash
sudo ./tests/test_io [limit_mbps] [read|write]
```

**Parâmetros:**
- `limit_mbps`: Limite de I/O em MB/s. Padrão: 10 MB/s
- `read|write`: Tipo de teste (leitura ou escrita). Padrão: write

**Exemplo:**
```bash
sudo ./tests/test_io 10 write    # Limitar escrita a 10 MB/s
sudo ./tests/test_io 50 read     # Limitar leitura a 50 MB/s
sudo ./tests/test_io 100 write   # Limitar escrita a 100 MB/s
```

**Saída:**
- Bytes lidos/escritos
- Throughput medido (MB/s)
- Desvio em relação ao limite configurado

---

## Requisitos

- **Sistema operacional:** Linux com cgroups v2
- **Privilégios:** root (sudo)
- **Dependências:** 
  - GCC
  - Make
  - Sistema de arquivos cgroup montado em `/sys/fs/cgroup`

## Verificar Suporte a cgroups v2

```bash
stat -fc %T /sys/fs/cgroup
# Deve retornar: cgroup2fs
```

## Compilação

Para compilar todos os testes:

```bash
make tests
```

Para limpar e recompilar:

```bash
make clean && make all && make tests
```

## Notas

- Os testes criam cgroups temporários que são removidos automaticamente ao final
- Arquivos temporários (para testes de I/O) são criados em `/tmp`
- Os resultados são exibidos em formato legível e também em CSV para fácil análise
- Cada teste executa por aproximadamente 10-15 segundos
