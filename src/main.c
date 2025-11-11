#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h> 
#include "../include/monitor.h"

volatile sig_atomic_t keep_running = 1;

// Handler para o sinal SIGINT (Ctrl+C).
void int_handler(int dummy) {
    (void)dummy; // Evita warning de 'parâmetro não utilizado'
    keep_running = 0;
}


int* parse_pids_argumento(const char *pids_str, int *num_pids) {
    if (!pids_str || strlen(pids_str) == 0) { *num_pids = 0; return NULL; }
    char *copia = malloc(strlen(pids_str) + 1);
    if (copia == NULL) { perror("malloc"); *num_pids = 0; return NULL; }
    strcpy(copia, pids_str);
    int *pids = malloc(10 * sizeof(int));
    if (pids == NULL) { free(copia); perror("malloc"); *num_pids = 0; return NULL; }
    *num_pids = 0;
    char *token = strtok(copia, ",");
    while (token != NULL && *num_pids < 10) { pids[(*num_pids)++] = atoi(token); token = strtok(NULL, ","); }
    free(copia);
    return pids;
}

int verificar_processo_existe(int pid) {
    char caminho[100];
    sprintf(caminho, "/proc/%d/stat", pid);
    FILE *arquivo = fopen(caminho, "r");
    if (arquivo) { fclose(arquivo); return 1; }
    return 0;
}

void mostrar_cabecalho(int num_pids, int intervalo) {
    printf("==============================  MONITORANDO %d PROCESSOS (Intervalo: %ds)  ==============================\n", num_pids, intervalo);
    printf("%-7s %-16s %-8s %-10s %-12s %-12s %-12s %-12s\n", "PID", "NOME", "CPU%", "MEMORIA", "I/O LEITURA", "I/O ESCRITA", "REDE RX", "REDE TX");
    printf("----------------------------------------------------------------------------------------------------------\n");
}

void atualizar_metricas_processo(ProcessMetrics *metrics, int intervalo) {
    if (!verificar_processo_existe(metrics->pid)) {
        metrics->cpu_usage = 0; metrics->memoria_mb = 0; metrics->io_read_rate = 0;
        metrics->io_write_rate = 0; metrics->net_rx_rate = 0; metrics->net_tx_rate = 0;
        strncpy(metrics->nome, "TERMINADO", sizeof(metrics->nome) - 1);
        metrics->nome[sizeof(metrics->nome) - 1] = '\0';
        return;
    }
    calcular_cpu_usage(metrics);
    metrics->memoria_mb = calcular_uso_memoria_mb(metrics->pid);
    obter_nome_processo(metrics->pid, metrics->nome, sizeof(metrics->nome));
    IOMetrics io_atual = obter_io_metrics(metrics->pid);
    calcular_taxas_io(&metrics->last_io, &io_atual, intervalo, &metrics->io_read_rate, &metrics->io_write_rate);
    metrics->last_io = io_atual;
    NetMetrics net_atual = obter_metricas_rede(metrics->pid);
    calcular_taxas_rede(&metrics->last_net, &net_atual, intervalo, &metrics->net_rx_rate, &metrics->net_tx_rate, &metrics->net_rx_packets_rate, &metrics->net_tx_packets_rate);
    metrics->last_net = net_atual;
    obter_context_switches_e_threads(metrics->pid, &metrics->voluntary_ctxt_switches, &metrics->nonvoluntary_ctxt_switches, &metrics->num_threads);
    unsigned long swap_kb_temp;
    obter_page_faults_e_swap(metrics->pid, &metrics->minor_page_faults, &metrics->major_page_faults, &swap_kb_temp);
    metrics->swap_mb = swap_kb_temp / 1024.0;
    unsigned long dummy_iowait;
    obter_syscalls_e_iowait(metrics->pid, &metrics->syscalls_read, &metrics->syscalls_write, &dummy_iowait);
}

void mostrar_metricas_processo(ProcessMetrics *metrics) {
    char read_str[32], write_str[32], net_rx_str[32], net_tx_str[32];
    formatar_taxa_io(metrics->io_read_rate, read_str, sizeof(read_str));
    formatar_taxa_io(metrics->io_write_rate, write_str, sizeof(write_str));
    formatar_taxa_rede(metrics->net_rx_rate, net_rx_str, sizeof(net_rx_str));
    formatar_taxa_rede(metrics->net_tx_rate, net_tx_str, sizeof(net_tx_str));
    if (strcmp(metrics->nome, "TERMINADO") == 0 || strcmp(metrics->nome, "NAO ENCONTRADO") == 0) {
        printf("%-7d %-16s %-8s %-10s %-12s %-12s %-12s %-12s\n", metrics->pid, metrics->nome, "N/A", "N/A", "N/A", "N/A", "N/A", "N/A");
    } else {
        printf("%-7d %-16.16s %-7.1f%%  %-9.1fMB %-12s %-12s %-12s %-12s\n", metrics->pid, metrics->nome, metrics->cpu_usage, metrics->memoria_mb, read_str, write_str, net_rx_str, net_tx_str);
    }
}

void mostrar_metricas_detalhadas_processo(ProcessMetrics *metrics, int num_procs) {
    char read_str[32], write_str[32], net_rx_str[32], net_tx_str[32];
    formatar_taxa_io(metrics->io_read_rate, read_str, sizeof(read_str));
    formatar_taxa_io(metrics->io_write_rate, write_str, sizeof(write_str));
    formatar_taxa_rede(metrics->net_rx_rate, net_rx_str, sizeof(net_rx_str));
    formatar_taxa_rede(metrics->net_tx_rate, net_tx_str, sizeof(net_tx_str));
    printf("--------------------------------------------------\n");
    printf("  PID: %-7d                NOME: %s\n", metrics->pid, metrics->nome);
    printf("--------------------------------------------------\n");
    printf("  CPU Usage: %-7.2f %%         Threads: %d\n", metrics->cpu_usage, metrics->num_threads);
    printf("  Memory (RSS): %-7.1f MB        Swap: %.1f MB\n", metrics->memoria_mb, metrics->swap_mb);
    printf("  Context Switches (Vol/Non): %lu / %lu\n", metrics->voluntary_ctxt_switches, metrics->nonvoluntary_ctxt_switches);
    printf("  Page Faults (Minor/Major): %lu / %lu\n", metrics->minor_page_faults, metrics->major_page_faults);
    printf("  I/O Read/Write Rate: %s / %s\n", read_str, write_str);
    printf("  Network RX/TX Rate: %s / %s\n", net_rx_str, net_tx_str);
    printf("  Syscalls Read/Write: %lu / %lu\n", metrics->syscalls_read, metrics->syscalls_write);
    if (num_procs > 1) { printf("\n"); }
}

void exportar_para_csv(ProcessMetrics *metrics, int num_pids, const char *filename, int primeira_vez) {
    FILE *csv = fopen(filename, primeira_vez ? "w" : "a");
    if (!csv) { perror("Erro ao abrir arquivo CSV"); return; }
    if (primeira_vez) {
        fprintf(csv, "Timestamp,PID,Nome,CPU_Percent,Memoria_MB,Swap_MB,IO_Read_Bps,IO_Write_Bps,Net_RX_Bps,Net_TX_Bps,Threads,Vol_Ctxt_Switches,NonVol_Ctxt_Switches,Minor_Page_Faults,Major_Page_Faults,Syscalls_Read,Syscalls_Write\n");
    }
    time_t now = time(NULL);
    for (int i = 0; i < num_pids; i++) {
        if (strcmp(metrics[i].nome, "TERMINADO") == 0 || strcmp(metrics[i].nome, "NAO ENCONTRADO") == 0) continue;
        fprintf(csv, "%ld,%d,%s,%.2f,%.2f,%.2f,%.0f,%.0f,%.0f,%.0f,%d,%lu,%lu,%lu,%lu,%lu,%lu\n",
                (long)now, metrics[i].pid, metrics[i].nome, metrics[i].cpu_usage, metrics[i].memoria_mb, metrics[i].swap_mb,
                metrics[i].io_read_rate, metrics[i].io_write_rate, metrics[i].net_rx_rate, metrics[i].net_tx_rate,
                metrics[i].num_threads, metrics[i].voluntary_ctxt_switches, metrics[i].nonvoluntary_ctxt_switches,
                metrics[i].minor_page_faults, metrics[i].major_page_faults, metrics[i].syscalls_read, metrics[i].syscalls_write);
    }
    fclose(csv);
}

void monitorar_multiplos_processos(int *pids, int num_pids, int intervalo, int modo_verbose, const char* csv_filename) {
    ProcessMetrics *metrics = calloc(num_pids, sizeof(ProcessMetrics));
    if (metrics == NULL) { perror("calloc"); return; }
    for (int i = 0; i < num_pids; i++) {
        metrics[i].pid = pids[i];
        if (verificar_processo_existe(pids[i])) {
            obter_nome_processo(pids[i], metrics[i].nome, sizeof(metrics[i].nome));
        } else {
            strncpy(metrics[i].nome, "NAO ENCONTRADO", sizeof(metrics[i].nome) - 1);
        }
    }
    if (csv_filename) { remove(csv_filename); }
    int iteracao = 0;

    // ALTERADO: O loop agora verifica a flag 'keep_running'. Ele vai parar quando
    // a flag se tornar 0 (após o usuário pressionar Ctrl+C).
    while (keep_running) {
        system("clear");
        for (int i = 0; i < num_pids; i++) {
             if (strcmp(metrics[i].nome, "NAO ENCONTRADO") != 0) {
                atualizar_metricas_processo(&metrics[i], intervalo);
             }
        }
        if (csv_filename) { exportar_para_csv(metrics, num_pids, csv_filename, iteracao == 0); }
        if (modo_verbose) {
            printf("===== MONITORAMENTO DETALHADO (Intervalo: %ds, Iteracao: %d) =====\n\n", intervalo, iteracao + 1);
            for (int i = 0; i < num_pids; i++) { mostrar_metricas_detalhadas_processo(&metrics[i], num_pids); }
        } else {
            mostrar_cabecalho(num_pids, intervalo);
            for (int i = 0; i < num_pids; i++) { mostrar_metricas_processo(&metrics[i]); }
        }
        printf("\n[Ctrl+C para sair]");
        if (csv_filename) { printf(" | Logando em: %s", csv_filename); }
        fflush(stdout);

        // ALTERADO: A função sleep() pode ser interrompida por um sinal. 
        // Em vez de um sleep complexo, vamos manter simples. Se o sinal for recebido,
        // o loop vai parar na próxima verificação de 'keep_running'.
        sleep(intervalo);
        iteracao++;
    }
    
    // NOVO: Este código agora é executado quando o loop termina, liberando a memória.
    printf("\n\nSinal de interrupção recebido. Liberando memória e saindo...\n");
    free(metrics);
}

void mostrar_uso_individual(int pid) {
    printf("\n=== ANALISE DETALHADA DO PROCESSO %d ===\n", pid);
    ProcessMetrics metric;
    memset(&metric, 0, sizeof(ProcessMetrics));
    metric.pid = pid;
    atualizar_metricas_processo(&metric, 1);
    sleep(1);
    atualizar_metricas_processo(&metric, 1);
    mostrar_metricas_detalhadas_processo(&metric, 1);
}

void mostrar_metricas_avancadas(int pid) {
    mostrar_uso_individual(pid);
    printf("\n=== INFORMACOES ADICIONAIS DO KERNEL ===\n");
    int priority, nice;
    obter_prioridade_nice(pid, &priority, &nice);
    printf("  Prioridade: %d, Nice: %d\n", priority, nice);
    NetworkConnections conns = obter_conexoes_rede(pid);
    printf("  Conexões TCP: %d (Listen: %d, Established: %d)\n", conns.tcp_connections, conns.tcp_listen, conns.tcp_established);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, int_handler);

    int *pids = NULL;
    int num_pids = 0;
    int intervalo = 2;
    int modo_individual = 0;
    int pid_individual = 0;
    int modo_avancado = 0;
    int modo_verbose = 0;
    char *csv_filename = NULL;
    
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("=== RESOURCE MONITOR - Monitoramento e Análise de Processos Linux ===\n\n"
               "USO:\n"
               "  %s --pids PID1,PID2,... [OPCOES]\n"
               "  %s --pid PID [OPCOES]\n"
               "  %s --list-ns PID\n"
               "  %s --compare-ns PID1,PID2\n\n"
               "OPCOES DE MONITORAMENTO:\n"
               "  --pids LISTA       Lista de PIDs para monitoramento contínuo\n"
               "  --intervalo N      Intervalo de atualização em segundos (padrao: 2)\n"
               "  --verbose, -v      Exibe todas as métricas detalhadas no modo contínuo\n"
               "  --csv ARQUIVO      Loga continuamente todas as métricas em um arquivo CSV\n\n"
               "OPCOES DE ANALISE (SNAPSHOT):\n"
               "  --pid PID          Exibe um snapshot único e detalhado de um processo\n"
               "  --avancado         (Use com --pid) Inclui ainda mais detalhes no snapshot\n"
               "  --list-ns PID      Lista todos os namespaces de um processo\n"
               "  --compare-ns PIDS  Compara os namespaces entre dois processos (ex: 123,456)\n"
               "  --find-ns PID TIPO Encontra processos no mesmo namespace (ex: 123 net)\n"
               "  --report-ns        Gera um relatório de todos os namespaces do sistema\n\n"
               "GERAL:\n"
               "  --help, -h         Mostrar esta ajuda\n", 
               argv[0], argv[0], argv[0], argv[0]);
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--pids") == 0 && i + 1 < argc) { pids = parse_pids_argumento(argv[i + 1], &num_pids); i++; } 
        else if (strcmp(argv[i], "--pid") == 0 && i + 1 < argc) { modo_individual = 1; pid_individual = atoi(argv[i + 1]); i++; } 
        else if (strcmp(argv[i], "--intervalo") == 0 && i + 1 < argc) { intervalo = atoi(argv[i + 1]); i++; } 
        else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) { modo_verbose = 1; } 
        else if (strcmp(argv[i], "--avancado") == 0) { modo_avancado = 1; } 
        else if (strcmp(argv[i], "--csv") == 0 && i + 1 < argc) { csv_filename = argv[i + 1]; i++; }
        
        else if (strcmp(argv[i], "--list-ns") == 0 && i + 1 < argc) {
            int pid_ns = atoi(argv[i+1]);
            if (pid_ns > 0) {
                listar_namespaces_processo(pid_ns);
            } else {
                fprintf(stderr, "Erro: PID inválido para --list-ns.\n");
            }
            return 0; // Termina o programa após a análise
        }
        else if (strcmp(argv[i], "--compare-ns") == 0 && i + 1 < argc) {
            comparar_namespaces(argv[i+1]);
            return 0; // Termina o programa após a análise
        }
        else if (strcmp(argv[i], "--find-ns") == 0 && i + 2 < argc) {
            int pid_ref = atoi(argv[i+1]);
            const char* ns_type = argv[i+2];
            encontrar_processos_no_namespace(pid_ref, ns_type);
            return 0;
        }
        // NOVO: Lógica para Tarefa 4
        else if (strcmp(argv[i], "--report-ns") == 0) {
            gerar_relatorio_namespaces_sistema();
            return 0;
        }
    }

    if (modo_individual) {
        if (!verificar_processo_existe(pid_individual)) { fprintf(stderr, "Erro: Processo %d nao encontrado!\n", pid_individual); return 1; }
        if (modo_avancado) { mostrar_metricas_avancadas(pid_individual); } 
        else { mostrar_uso_individual(pid_individual); }
    } else if (num_pids > 0) {
        monitorar_multiplos_processos(pids, num_pids, intervalo, modo_verbose, csv_filename);
    } else {
        fprintf(stderr, "Erro: Nenhuma ação especificada. Use --help para ver as opções.\n");
        if (pids != NULL) { free(pids); }
        return 1;
    }

    if (pids != NULL) {
        free(pids);
    }
    return 0;
}