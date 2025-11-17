# 🧪 Guia Rápido - Experimentos Obrigatórios

## 🎯 O que fazer agora

Você tem **5 experimentos obrigatórios** prontos para execução. Todos os scripts estão configurados e testados.

## 🚀 Execução Rápida (3 passos)

### 1. Compile o projeto (se ainda não fez)

```bash
cd /home/yas/resource-monitor
make
```

### 2. Execute o script mestre

```bash
cd scripts
./run_experiments.sh
```

### 3. Escolha a opção 6 (Executar TODOS)

O script irá:
- ✅ Executar os 5 experimentos em sequência
- ✅ Gerar relatórios individuais
- ✅ Criar relatório consolidado
- ✅ Salvar todos os dados em CSV/TXT

**Tempo estimado**: 5-10 minutos

## 📋 Checklist Antes de Executar

- [ ] Projeto compilado (`make` executado)
- [ ] `stress` instalado (`sudo apt install stress`)
- [ ] Espaço em disco (pelo menos 1GB livre)
- [ ] **Sistema usa cgroups v2** (Ubuntu 22.04+)

Verificar cgroups v2:
```bash
stat -fc %T /sys/fs/cgroup
# Deve retornar: cgroup2fs
```

## 📊 Estrutura dos Resultados

Após execução, você terá:

```
experimentos/
├── README.md                          ← Documentação completa
├── RELATORIO_CONSOLIDADO.md           ←  RELATÓRIO PRINCIPAL
│
├── exp1_overhead/
│   ├── RELATORIO.md                   ← Análise do experimento 1
│   └── *.csv                          ← Dados brutos
│
├── exp2_namespaces/
│   ├── RELATORIO.md                   ← Análise do experimento 2
│   └── *.txt, *.csv
│
├── exp3_cpu_throttling/
│   ├── RELATORIO.md                   ← Análise do experimento 3
│   └── *.csv
│
├── exp4_memory_limit/
│   ├── RELATORIO.md                   ← Análise do experimento 4
│   └── *.txt, *.csv
│
└── exp5_io_limit/
    ├── RELATORIO.md                   ← Análise do experimento 5
    └── *.csv
```

##  Visualizar Resultados

### Ver relatório consolidado:
```bash
cat experimentos/RELATORIO_CONSOLIDADO.md
```

### Ver experimento específico:
```bash
cat experimentos/exp1_overhead/RELATORIO.md
```

### Ver dados brutos:
```bash
cat experimentos/exp3_cpu_throttling/cpu_throttling_results.csv
```

## 🎓 O que cada experimento faz

| # | Experimento | O que testa | Tempo |
|---|-------------|-------------|-------|
| 1 | **Overhead de Monitoramento** | Impacto do profiler no sistema | ~2 min |
| 2 | **Isolamento via Namespaces** | Efetividade dos namespaces Linux | ~1 min |
| 3 | **Throttling de CPU** | Precisão de limites de CPU | ~2 min |
| 4 | **Limitação de Memória** | Comportamento ao atingir limite RAM | ~1 min |
| 5 | **Limitação de I/O** | Controle de throughput de disco | ~2 min |

## Execução Individual

Se preferir executar um por vez:

```bash
# Exp 1 (não precisa sudo)
./scripts/experimento1_overhead.sh

# Exp 2-5 (precisam sudo)
sudo ./scripts/experimento2_namespaces.sh
sudo ./scripts/experimento3_cpu_throttling.sh
sudo ./scripts/experimento4_memory_limit.sh
sudo ./scripts/experimento5_io_limit.sh
```

## Problemas Comuns

### "stress: command not found"
```bash
sudo apt install stress
```

### "Sistema não parece usar cgroups v2"
Seu kernel usa cgroups v1. Opções:
1. Usar Ubuntu 22.04+ (recomendado)
2. Executar em VM/container com cgroups v2

### "Permission denied"
Use `sudo` para experimentos 2-5:
```bash
sudo ./scripts/run_experiments.sh
```

### Experimento 5 não aplica limites
Normal se usar tmpfs ou certos SSDs. O script irá avisar.

## Documentação Completa

- **Este guia**: `EXPERIMENTOS_QUICKSTART.md`
- **Documentação detalhada**: `experimentos/README.md`
- **README do projeto**: `README.md`