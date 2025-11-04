# Resource Monitor

## Descrição do Projeto
Sistema de monitoramento e análise de recursos para processos e containers Linux, desenvolvido em C.

## Funcionalidades

### 1. Resource Profiler
- Monitorar processo por PID com intervalo configurável
- Coletar CPU, Memória, I/O
- Calcular CPU% e taxas de I/O
- Exportar dados em CSV
- Tratar erros (processo inexistente, permissões)
- **+ Suporte a múltiplos processos simultaneamente**
- **+ Comparação com ferramentas existentes (docker stats)**

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

## Qualidade de Código
- Compilar sem warnings (-Wall -Wextra)
- Código comentado e bem estruturado
- Makefile funcional
- README com instruções de compilação e uso
- **+ Sem memory leaks (validado com valgrind)**

---

**OBS: Se der tempo, implementar dashboard web para visualização de métricas**

---
## Requisitos e Dependências
- **Sistema:** Ubuntu 24.04 LTS
- **Kernel:** Linux 6.6.87.2-microsoft-standard-WSL2  
- **Compilador:** GCC 11.4.0+
- **Ferramentas de desenvolvimento:** strace, gdb, valgrind
- **Documentação:** man-pages (proc, cgroups, namespaces)