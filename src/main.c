#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "../include/monitor.h"

// === FUNÇÕES QUE ESTAVAM FALTANDO ===

int* parse_pids_argumento(const char *pids_str, int *num_pids) {
    // Substituir strdup por uma implementação manual
    char *copia = malloc(strlen(pids_str) + 1);
    if (copia == NULL) {
        *num_pids = 0;
        return NULL;
    }
    strcpy(copia, pids_str);
    
    char *token;
    int *pids = malloc(10 * sizeof(int));
    *num_pids = 0;
    
    token = strtok(copia, ",");
    while (token != NULL && *num_pids < 10) {
        pids[(*num_pids)++] = atoi(token);
        token = strtok(NULL, ",");
    }
    
    free(copia);
    return pids;
}

int verificar_processo_existe(int pid) {
    char caminho[100];
    sprintf(caminho, "/proc/%d/stat", pid);
    FILE *arquivo = fopen(caminho, "r");
    if (arquivo) {
        fclose(arquivo);
        return 1;
    }
    return 0;
}

void mostrar_cabecalho(int num_pids, int intervalo) {
    printf("===========  MONITORANDO %d PROCESSOS (Intervalo: %ds)  ===========\n", num_pids, intervalo);
    printf("PID     NOME           CPU%%    MEMORIA    I/O LEITURA  I/O ESCRITA  REDE RX      REDE TX\n");
    printf("-----------------------------------------------------------------------------------------\n");
}

void atualizar_metricas_processo(ProcessMetrics *metrics, int intervalo) {
    if (!verificar_processo_existe(metrics->pid)) {
        metrics->cpu_usage = 0;
        metrics->memoria_mb = 0;
        metrics->io_read_rate = 0;
        metrics->io_write_rate = 0;
        metrics->net_rx_rate = 0;
        metrics->net_tx_rate = 0;
        metrics->net_rx_packets_rate = 0;
        metrics->net_tx_packets_rate = 0;
        strcpy(metrics->nome, "TERMINADO");
        return;
    }
    
    metrics->cpu_usage = calcular_cpu_usage_contorno(metrics->pid, &metrics->last_cpu_time);
    metrics->memoria_mb = calcular_uso_memoria_mb(metrics->pid);
    
    IOMetrics io_atual = obter_io_metrics(metrics->pid);
    calcular_taxas_io(&metrics->last_io, &io_atual, intervalo, &metrics->io_read_rate, &metrics->io_write_rate);
    metrics->last_io = io_atual;
    
    NetMetrics net_atual = obter_metricas_rede(metrics->pid);
    calcular_taxas_rede(&metrics->last_net, &net_atual, intervalo, 
                       &metrics->net_rx_rate, &metrics->net_tx_rate,
                       &metrics->net_rx_packets_rate, &metrics->net_tx_packets_rate);
    metrics->last_net = net_atual;
    
    obter_nome_processo(metrics->pid, metrics->nome, sizeof(metrics->nome));
}

void mostrar_metricas_processo(ProcessMetrics *metrics) {
    char read_str[32], write_str[32];
    char net_rx_str[32], net_tx_str[32];
    
    formatar_taxa_io(metrics->io_read_rate, read_str, sizeof(read_str));
    formatar_taxa_io(metrics->io_write_rate, write_str, sizeof(write_str));
    formatar_taxa_rede(metrics->net_rx_rate, net_rx_str, sizeof(net_rx_str));
    formatar_taxa_rede(metrics->net_tx_rate, net_tx_str, sizeof(net_tx_str));
    
    if (strcmp(metrics->nome, "TERMINADO") == 0) {
        printf("%-7d %-14s %-6s   %-9s  %-11s  %-11s  %-11s  %-11s\n", 
               metrics->pid, metrics->nome, "N/A", "N/A", "N/A", "N/A", "N/A", "N/A");
    } else {
        printf("%-7d %-14s %-6.1f   %-6.1fMB   %-11s  %-11s  %-11s  %-11s\n", 
               metrics->pid, metrics->nome, metrics->cpu_usage, 
               metrics->memoria_mb, read_str, write_str, net_rx_str, net_tx_str);
    }
}

void mostrar_uso_individual(int pid) {
    printf("\n=== ANALISE DETALHADA DO PROCESSO %d ===\n", pid);
    
    // CPU
    calcular_cpu_usage(pid);
    
    // Memória
    printf("\n=== INFORMACOES DE MEMORIA ===\n");
    double memoria = calcular_uso_memoria_mb(pid);
    unsigned long vsz = obter_vsz(pid);
    printf("Uso de memoria RSS: %.1f MB\n", memoria);
    printf("Uso de memoria VSZ: %.1f MB\n", (vsz * sysconf(_SC_PAGESIZE)) / (1024.0 * 1024.0));
    
    // I/O
    printf("\n=== INFORMACOES DE I/O ===\n");
    IOMetrics io = obter_io_metrics(pid);
    printf("Bytes lidos: %lu\n", io.read_bytes);
    printf("Bytes escritos: %lu\n", io.write_bytes);
    printf("Chars lidos: %lu\n", io.read_chars);
    printf("Chars escritos: %lu\n", io.write_chars);
    
    // Rede
    printf("\n=== INFORMACOES DE REDE ===\n");
    NetMetrics net = obter_metricas_rede(pid);
    printf("Bytes recebidos: %lu\n", net.rx_bytes);
    printf("Bytes enviados: %lu\n", net.tx_bytes);
    printf("Pacotes recebidos: %lu\n", net.packets_rx);
    printf("Pacotes enviados: %lu\n", net.packets_tx);
    
    NetworkConnections conns = obter_conexoes_rede(pid);
    printf("Conexões TCP: %d (Listen: %d, Established: %d)\n", 
           conns.tcp_connections, conns.tcp_listen, conns.tcp_established);
}

void exportar_para_csv(ProcessMetrics *metrics, int num_pids, const char *filename) {
    FILE *csv = fopen(filename, "w");
    if (!csv) {
        printf("Erro ao criar arquivo CSV: %s\n", filename);
        return;
    }
    
    fprintf(csv, "Timestamp,PID,Nome,CPU%%,Memoria_MB,IO_Read_Rate,IO_Write_Rate,Net_RX_Rate,Net_TX_Rate\n");
    
    time_t now = time(NULL);
    for (int i = 0; i < num_pids; i++) {
        fprintf(csv, "%ld,%d,%s,%.1f,%.1f,%.0f,%.0f,%.0f,%.0f\n",
                now, metrics[i].pid, metrics[i].nome, metrics[i].cpu_usage,
                metrics[i].memoria_mb, metrics[i].io_read_rate, metrics[i].io_write_rate,
                metrics[i].net_rx_rate, metrics[i].net_tx_rate);
    }
    
    fclose(csv);
    printf("Dados exportados para: %s\n", filename);
}

void monitorar_multiplos_processos(int *pids, int num_pids, int intervalo) {
    int iteracao = 0;
    ProcessMetrics *metrics = malloc(num_pids * sizeof(ProcessMetrics));
    
    // Inicializar métricas
    for (int i = 0; i < num_pids; i++) {
        metrics[i].pid = pids[i];
        metrics[i].last_io = obter_io_metrics(pids[i]);
        metrics[i].last_net = obter_metricas_rede(pids[i]);
        metrics[i].last_cpu_time = 0;
        metrics[i].cpu_usage = 0;
        metrics[i].memoria_mb = 0;
        metrics[i].io_read_rate = 0;
        metrics[i].io_write_rate = 0;
        metrics[i].net_rx_rate = 0;
        metrics[i].net_tx_rate = 0;
        obter_nome_processo(pids[i], metrics[i].nome, sizeof(metrics[i].nome));
        
        // Primeira leitura para inicializar
        if (verificar_processo_existe(pids[i])) {
            calcular_cpu_usage_contorno(pids[i], &metrics[i].last_cpu_time);
        }
    }
    
    // Aguardar um ciclo antes de começar o loop
    sleep(1);
    
    while (1) {
        system("clear");
        mostrar_cabecalho(num_pids, intervalo);
        
        for (int i = 0; i < num_pids; i++) {
            atualizar_metricas_processo(&metrics[i], intervalo);
            mostrar_metricas_processo(&metrics[i]);
        }
        
        printf("\nIteracao: %d - [Ctrl+C para sair]", ++iteracao);
        
        fflush(stdout);
        sleep(intervalo);
    }
    
    free(metrics);
}

void mostrar_metricas_avancadas(int pid) {
    // Declarações das funções dos outros módulos
    extern void mostrar_metricas_cpu_avancadas(int pid);
    extern void mostrar_metricas_memoria_avancadas(int pid);
    extern void mostrar_metricas_io_avancadas(int pid);
    extern void mostrar_metricas_rede_avancadas(int pid);
    
    printf("\n=== METRICAS AVANCADAS DO PROCESSO %d ===\n", pid);
    mostrar_metricas_cpu_avancadas(pid);
    mostrar_metricas_memoria_avancadas(pid);
    mostrar_metricas_io_avancadas(pid);
    mostrar_metricas_rede_avancadas(pid);
}

// === FUNÇÃO MAIN ===

int main(int argc, char *argv[]) {
    int *pids = NULL;
    int num_pids = 0;
    int intervalo = 2;
    int modo_individual = 0;
    int pid_individual = 0;
    int modo_avancado = 0;
    int modo_csv = 0;
    char *csv_filename = NULL;
    
    // === SISTEMA DE HELP MELHORADO ===
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("=== RESOURCE MONITOR - Monitoramento de Processos Linux ===\n\n");
        printf("USO:\n");
        printf("  %s --pid PID [OPCOES]           # Monitorar processo individual\n", argv[0]);
        printf("  %s --pids PID1,PID2,... [OPCOES] # Monitorar múltiplos processos\n", argv[0]);
        printf("\nOPCOES:\n");
        printf("  --pid PID          Monitorar processo específico\n");
        printf("  --pids LISTA       Lista de PIDs separados por vírgula (max 10)\n");
        printf("  --intervalo N      Intervalo de atualização em segundos (1-60, padrao: 2)\n");
        printf("  --detalhes         Modo detalhado (apenas com --pid)\n");
        printf("  --avancado         Métricas avançadas (apenas com --pid)\n");
        printf("  --csv ARQUIVO      Exportar dados para CSV\n");
        printf("  --help, -h         Mostrar esta ajuda\n");
        printf("\nEXEMPLOS:\n");
        printf("  %s --pid 1 --detalhes\n", argv[0]);
        printf("  %s --pid 1234 --avancado\n", argv[0]);
        printf("  %s --pids 1,551,$$ --intervalo 3\n", argv[0]);
        printf("  %s --pids 1,551 --csv dados.csv\n", argv[0]);
        printf("  %s --pids 1,551 --intervalo 5 --csv monitoramento.csv\n", argv[0]);
        printf("\nDESCRICAO DAS METRICAS:\n");
        printf("  CPU%%      Uso de CPU (como 'top')\n");
        printf("  MEMORIA   Uso de memória RSS em MB\n");
        printf("  I/O       Taxas de leitura/escrita em B/s, KB/s ou MB/s\n");
        printf("  REDE      Taxas de rede RX/TX em B/s, KB/s ou MB/s\n");
        printf("\nMETRICAS AVANCADAS:\n");
        printf("  - Context switches (voluntary/non-voluntary)\n");
        printf("  - Threads, Page faults (minor/major), Swap usage\n");
        printf("  - Syscalls, I/O wait time, Prioridade/Nice value\n");
        printf("  - Conexões TCP/UDP, Estatísticas de rede detalhadas\n");
        return 0;
    }
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--pids") == 0 && i + 1 < argc) {
            pids = parse_pids_argumento(argv[i + 1], &num_pids);
        } else if (strcmp(argv[i], "--pid") == 0 && i + 1 < argc) {
            modo_individual = 1;
            pid_individual = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "--intervalo") == 0 && i + 1 < argc) {
            intervalo = atoi(argv[i + 1]);
            if (intervalo < 1) intervalo = 1;
            if (intervalo > 60) intervalo = 60;
        } else if (strcmp(argv[i], "--detalhes") == 0) {
            // Modo detalhado já é o padrão para --pid
        } else if (strcmp(argv[i], "--avancado") == 0) {
            modo_avancado = 1;
        } else if (strcmp(argv[i], "--csv") == 0 && i + 1 < argc) {
            modo_csv = 1;
            csv_filename = argv[i + 1];
        }
    }
    
    if (modo_individual) {
        if (!verificar_processo_existe(pid_individual)) {
            printf("Erro: Processo %d nao encontrado!\n", pid_individual);
            return 1;
        }
        
        if (modo_avancado) {
            mostrar_metricas_avancadas(pid_individual);
        } else {
            mostrar_uso_individual(pid_individual);
        }
    } else {
        if (pids == NULL || num_pids == 0) {
            printf("Erro: Nenhum PID válido especificado\n");
            printf("Use --pids PID1,PID2,... ou --pid PID\n");
            return 1;
        }
        
        // Verificar se todos os PIDs existem
        for (int i = 0; i < num_pids; i++) {
            if (!verificar_processo_existe(pids[i])) {
                printf("Aviso: Processo %d nao encontrado!\n", pids[i]);
            }
        }
        
        if (modo_csv) {
            // Modo único com exportação CSV
            ProcessMetrics *metrics = malloc(num_pids * sizeof(ProcessMetrics));
            for (int i = 0; i < num_pids; i++) {
                metrics[i].pid = pids[i];
                metrics[i].last_io = obter_io_metrics(pids[i]);
                metrics[i].last_net = obter_metricas_rede(pids[i]);
                metrics[i].last_cpu_time = 0;
                obter_nome_processo(pids[i], metrics[i].nome, sizeof(metrics[i].nome));
                
                // Atualizar métricas uma vez
                atualizar_metricas_processo(&metrics[i], 1);
            }
            exportar_para_csv(metrics, num_pids, csv_filename);
            free(metrics);
        } else {
            // Modo monitoramento contínuo
            monitorar_multiplos_processos(pids, num_pids, intervalo);
        }
    }
    
    if (pids != NULL) {
        free(pids);
    }
    
    return 0;
}