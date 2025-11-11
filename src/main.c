#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "monitor.h"
#include "namespace.h"

#define MAX_PIDS 128

volatile sig_atomic_t keep_running = 1;

void int_handler(int dummy) {
    (void)dummy;
    keep_running = 0;
}

int* parse_pids_argumento(const char *pids_str, int *num_pids) {
    if (!pids_str || strlen(pids_str) == 0) { *num_pids = 0; return NULL; }
    
    char *copia = strdup(pids_str);
    if (copia == NULL) { *num_pids = 0; return NULL; }

    int *pids = malloc(MAX_PIDS * sizeof(int));
    if (pids == NULL) { free(copia); *num_pids = 0; return NULL; }
    
    *num_pids = 0;
    char *token = strtok(copia, ",");
    while (token != NULL && *num_pids < MAX_PIDS) {
        pids[(*num_pids)++] = atoi(token);
        token = strtok(NULL, ",");
    }
    
    free(copia);
    return pids;
}

void mostrar_cabecalho(int num_pids, int intervalo) {
    printf("======================== MONITORANDO %d PROCESSO(S) (Intervalo: %ds) ========================\n", num_pids, intervalo);
    printf("%-7s %-20s %-9s %-10s %-12s %-12s %-12s %-12s\n", "PID", "NOME", "CPU %", "MEM (MB)", "I/O LEITURA", "I/O ESCRITA", "REDE RX", "REDE TX");
    printf("-----------------------------------------------------------------------------------------------------------\n");
}

void atualizar_metricas_processo(ProcessMetrics *metrics, double intervalo_sec) {
    if (!verificar_processo_existe(metrics->pid)) {
        strncpy(metrics->nome, "[ terminado ]", sizeof(metrics->nome) - 1);
        metrics->cpu_usage = 0;
        metrics->memoria_mb = 0;
        metrics->io_read_rate = 0;
        metrics->io_write_rate = 0;
        metrics->net_rx_rate = 0;
        metrics->net_tx_rate = 0;
        return;
    }

    obter_nome_processo(metrics->pid, metrics->nome, sizeof(metrics->nome));
    calcular_cpu_usage(metrics);
    metrics->memoria_mb = calcular_uso_memoria_mb(metrics->pid);

    IOMetrics io_atual = obter_io_metrics(metrics->pid);
    calcular_taxas_io(&metrics->last_io, &io_atual, intervalo_sec, &metrics->io_read_rate, &metrics->io_write_rate);
    metrics->last_io = io_atual;

    NetMetrics net_atual = obter_metricas_rede(metrics->pid);
    calcular_taxas_rede(&metrics->last_net, &net_atual, intervalo_sec, &metrics->net_rx_rate, &metrics->net_tx_rate);
    metrics->last_net = net_atual;

    obter_context_switches_e_threads(metrics->pid, &metrics->voluntary_ctxt_switches, &metrics->nonvoluntary_ctxt_switches, &metrics->num_threads);
    unsigned long swap_kb_temp;
    obter_page_faults_e_swap(metrics->pid, &metrics->minor_page_faults, &metrics->major_page_faults, &swap_kb_temp);
    metrics->swap_mb = swap_kb_temp / 1024.0;
    obter_syscalls(metrics->pid, &metrics->syscalls_read, &metrics->syscalls_write);
}

void mostrar_metricas_processo(ProcessMetrics *metrics) {
    if (strcmp(metrics->nome, "[ não encontrado ]") == 0 || strcmp(metrics->nome, "[ terminado ]") == 0) {
        printf("%-7d %-20.20s %-9s %-10s %-12s %-12s %-12s %-12s\n", 
               metrics->pid, metrics->nome, "N/A", "N/A", "N/A", "N/A", "N/A", "N/A");
        return;
    }
    
    char read_str[32], write_str[32], net_rx_str[32], net_tx_str[32];
    formatar_taxa(metrics->io_read_rate, read_str, sizeof(read_str));
    formatar_taxa(metrics->io_write_rate, write_str, sizeof(write_str));
    formatar_taxa(metrics->net_rx_rate, net_rx_str, sizeof(net_rx_str));
    formatar_taxa(metrics->net_tx_rate, net_tx_str, sizeof(net_tx_str));

    printf("%-7d %-20.20s %-8.2f%% %-9.1fMB %-12s %-12s %-12s %-12s\n", 
           metrics->pid, metrics->nome, metrics->cpu_usage, metrics->memoria_mb, 
           read_str, write_str, net_rx_str, net_tx_str);
}

void mostrar_metricas_detalhadas(ProcessMetrics *metrics) {
    printf("----------------------------------------\n");
    printf("  PID: %d | NOME: %s\n", metrics->pid, metrics->nome);
    printf("----------------------------------------\n");

    if (strcmp(metrics->nome, "[ não encontrado ]") == 0 || strcmp(metrics->nome, "[ terminado ]") == 0) {
        printf("  Processo não está mais ativo.\n\n");
        return;
    }

    char read_str[32], write_str[32], net_rx_str[32], net_tx_str[32];
    formatar_taxa(metrics->io_read_rate, read_str, sizeof(read_str));
    formatar_taxa(metrics->io_write_rate, write_str, sizeof(write_str));
    formatar_taxa(metrics->net_rx_rate, net_rx_str, sizeof(net_rx_str));
    formatar_taxa(metrics->net_tx_rate, net_tx_str, sizeof(net_tx_str));
    
    printf("  CPU Usage: %.2f %%         Threads: %d\n", metrics->cpu_usage, metrics->num_threads);
    printf("  Memory (RSS): %.1f MB        Swap: %.1f MB\n", metrics->memoria_mb, metrics->swap_mb);
    printf("  Context Switches (Vol/Non): %lu / %lu\n", metrics->voluntary_ctxt_switches, metrics->nonvoluntary_ctxt_switches);
    printf("  Page Faults (Minor/Major): %lu / %lu\n", metrics->minor_page_faults, metrics->major_page_faults);
    printf("  I/O Read/Write Rate: %s / %s\n", read_str, write_str);
    printf("  Network RX/TX Rate: %s / %s\n", net_rx_str, net_tx_str);
    printf("  Syscalls Read/Write: %lu / %lu\n\n", metrics->syscalls_read, metrics->syscalls_write);
}

void exportar_para_csv(ProcessMetrics *metrics, int num_pids, const char *filename, int primeira_vez) {
    FILE *csv = fopen(filename, primeira_vez ? "w" : "a");
    if (!csv) { perror("Erro ao abrir arquivo CSV"); return; }
    
    if (primeira_vez) {
        fprintf(csv, "Timestamp,PID,Nome,CPU_Percent,Memoria_MB,Swap_MB,IO_Read_Bps,IO_Write_Bps,Net_RX_Bps,Net_TX_Bps,Threads,Vol_Ctxt_Switches,NonVol_Ctxt_Switches,Minor_Page_Faults,Major_Page_Faults,Syscalls_Read,Syscalls_Write\n");
    }
    
    time_t now = time(NULL);
    for (int i = 0; i < num_pids; i++) {
        if (strcmp(metrics[i].nome, "[ terminado ]") == 0 || strcmp(metrics[i].nome, "[ não encontrado ]") == 0) continue;
        fprintf(csv, "%ld,%d,%s,%.2f,%.2f,%.2f,%.0f,%.0f,%.0f,%.0f,%d,%lu,%lu,%lu,%lu,%lu,%lu\n",
                (long)now, metrics[i].pid, metrics[i].nome, metrics[i].cpu_usage, metrics[i].memoria_mb, metrics[i].swap_mb,
                metrics[i].io_read_rate, metrics[i].io_write_rate, metrics[i].net_rx_rate, metrics[i].net_tx_rate,
                metrics[i].num_threads, metrics[i].voluntary_ctxt_switches, metrics[i].nonvoluntary_ctxt_switches,
                metrics[i].minor_page_faults, metrics[i].major_page_faults, metrics[i].syscalls_read, metrics[i].syscalls_write);
    }
    fclose(csv);
}

void monitorar_processos(int *pids, int num_pids, int intervalo, int modo_verbose, const char* csv_filename) {
    ProcessMetrics *metrics = calloc(num_pids, sizeof(ProcessMetrics));
    if (metrics == NULL) { return; }

    for (int i = 0; i < num_pids; i++) {
        metrics[i].pid = pids[i];
        if (!verificar_processo_existe(pids[i])) {
            strncpy(metrics[i].nome, "[ não encontrado ]", sizeof(metrics[i].nome) - 1);
        }
    }

    if (csv_filename) { exportar_para_csv(NULL, 0, csv_filename, 1); }
    int iteracao = 0;

    while (keep_running) {
        struct timespec start_time;
        clock_gettime(CLOCK_MONOTONIC, &start_time);

        for (int i = 0; i < num_pids; i++) {
             if (strcmp(metrics[i].nome, "[ não encontrado ]") != 0) {
                double intervalo_real_sec = (double)intervalo;
                if (iteracao > 0) {
                    intervalo_real_sec = (start_time.tv_sec - metrics[i].last_time_snapshot.tv_sec) + 
                                         (start_time.tv_nsec - metrics[i].last_time_snapshot.tv_nsec) / 1e9;
                }
                atualizar_metricas_processo(&metrics[i], intervalo_real_sec);
             }
        }
        
        if (csv_filename) { exportar_para_csv(metrics, num_pids, csv_filename, iteracao == 0); }
        
        system("clear");
        if (modo_verbose) {
            printf("===== MONITORAMENTO DETALHADO (Iteração: %d) =====\n\n", iteracao + 1);
            for (int i = 0; i < num_pids; i++) { mostrar_metricas_detalhadas(&metrics[i]); }
        } else {
            mostrar_cabecalho(num_pids, intervalo);
            for (int i = 0; i < num_pids; i++) { mostrar_metricas_processo(&metrics[i]); }
        }
        
        printf("\n[Pressione Ctrl+C para sair]");
        if (csv_filename) { printf(" | Logando em: %s", csv_filename); }
        fflush(stdout);

        iteracao++;
        sleep(intervalo);
    }
    
    printf("\n\nSinal de interrupção recebido. Encerrando...\n");
    free(metrics);
}

void mostrar_ajuda(const char* prog_name) {
    printf("Uso: %s [OPÇÕES]\n\n"
           "OPÇÕES DE MONITORAMENTO:\n"
           "  --pids PID1,PID2,...  Lista de PIDs para monitoramento contínuo (obrigatório para monitorar).\n"
           "  --intervalo N         Intervalo de atualização em segundos (padrão: 2).\n"
           "  --verbose, -v         Exibe todas as métricas detalhadas no modo contínuo.\n"
           "  --csv ARQUIVO         Loga continuamente todas as métricas em um arquivo CSV.\n\n"
           "OPÇÕES DE ANÁLISE DE NAMESPACE:\n"
           "  --list-ns PID         Lista todos os namespaces de um processo.\n"
           "  --compare-ns PIDS     Compara os namespaces entre dois processos (ex: 123,456).\n"
           "  --find-ns PID TIPO    Encontra processos no mesmo namespace (ex: 123 net).\n"
           "  --report-ns           Gera um relatório de todos os namespaces do sistema.\n\n"
           "GERAL:\n"
           "  --help, -h            Mostra esta ajuda.\n",
           prog_name);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, int_handler);

    if (argc < 2) {
        mostrar_ajuda(argv[0]);
        return 1;
    }

    char *pids_str = NULL;
    int intervalo = 2;
    int modo_verbose = 0;
    char *csv_filename = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            mostrar_ajuda(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--pids") == 0 && i + 1 < argc) {
            pids_str = argv[++i];
        } else if (strcmp(argv[i], "--intervalo") == 0 && i + 1 < argc) {
            intervalo = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            modo_verbose = 1;
        } else if (strcmp(argv[i], "--csv") == 0 && i + 1 < argc) {
            csv_filename = argv[++i];
        } else if (strcmp(argv[i], "--list-ns") == 0 && i + 1 < argc) {
            listar_namespaces_processo(atoi(argv[++i]));
            return 0;
        } else if (strcmp(argv[i], "--compare-ns") == 0 && i + 1 < argc) {
            comparar_namespaces(argv[++i]);
            return 0;
        } else if (strcmp(argv[i], "--find-ns") == 0 && i + 2 < argc) {
            encontrar_processos_no_namespace(atoi(argv[i+1]), argv[i+2]);
            i += 2;
            return 0;
        } else if (strcmp(argv[i], "--report-ns") == 0) {
            gerar_relatorio_namespaces_sistema();
            return 0;
        }
    }

    if (pids_str) {
        int num_pids = 0;
        int *pids = parse_pids_argumento(pids_str, &num_pids);
        if (num_pids > 0) {
            monitorar_processos(pids, num_pids, intervalo, modo_verbose, csv_filename);
            free(pids);
        } else {
            fprintf(stderr, "Erro: Nenhum PID válido fornecido com --pids.\n");
            free(pids);
            return 1;
        }
    } else {
        fprintf(stderr, "Nenhuma ação especificada. Use --pids para monitorar ou --help para ver outras opções.\n");
        return 1;
    }

    return 0;
}