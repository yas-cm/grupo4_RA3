#ifndef MONITOR_H
#define MONITOR_H

#include <time.h>
#include <stddef.h>

/* Constantes de Configuração */
#define PROC_PATH_MAX 256
#define PROC_LINE_MAX 1024
#define NS_ID_MAX 256
#define CGROUP_PATH_MAX 512
#define PROCESS_NAME_MAX 256

/* Constantes para Média Móvel Exponencial (EMA) de CPU */
#define CPU_EMA_ALPHA 0.3        /* Peso para valor novo (30%) */
#define CPU_EMA_BETA (1.0 - CPU_EMA_ALPHA)  /* Peso para valor antigo (70%) */
#define CPU_DECAY_FACTOR 0.7     /* Fator de decaimento quando processo some */
#define CPU_MIN_THRESHOLD 0.1    /* Threshold mínimo para considerar CPU = 0 */
#define TIME_DIFF_MIN 0.01       /* Intervalo mínimo em segundos para cálculo */

/* Constantes para formatação de taxas */
#define BYTES_PER_KB 1024
#define BYTES_PER_MB (1024 * 1024)

typedef struct {
    unsigned long read_bytes;
    unsigned long write_bytes;
} IOMetrics;

typedef struct {
    unsigned long rx_bytes;
    unsigned long tx_bytes;
    unsigned long packets_rx;
    unsigned long packets_tx;
} NetMetrics;

typedef struct {
    int pid;
    char nome[PROCESS_NAME_MAX];
    
    double cpu_usage;
    double memoria_mb;
    double io_read_rate;
    double io_write_rate;
    double net_rx_rate;
    double net_tx_rate;

    int num_threads;
    unsigned long voluntary_ctxt_switches;
    unsigned long nonvoluntary_ctxt_switches;
    unsigned long minor_page_faults;
    unsigned long major_page_faults;
    double swap_mb;
    unsigned long syscalls_read;
    unsigned long syscalls_write;

    IOMetrics last_io;
    NetMetrics last_net;
    unsigned long last_total_jiffies;
    struct timespec last_time_snapshot;

} ProcessMetrics;

typedef struct {
    int tcp_connections;
    int udp_connections;
    int tcp_listen;
    int tcp_established;
} NetworkConnections;

void calcular_cpu_usage(ProcessMetrics *metrics);
double calcular_uso_memoria_mb(int pid);
void obter_page_faults_e_swap(int pid, unsigned long *minor_faults, unsigned long *major_faults, unsigned long *swap_kb);

IOMetrics obter_io_metrics(int pid);
void calcular_taxas_io(const IOMetrics *antes, const IOMetrics *depois, double intervalo_sec, double *read_rate, double *write_rate);

NetMetrics obter_metricas_rede(int pid);
void calcular_taxas_rede(const NetMetrics *antes, const NetMetrics *depois, double intervalo_sec, double *rx_rate, double *tx_rate);

NetworkConnections obter_conexoes_rede(int pid);
void obter_context_switches_e_threads(int pid, unsigned long *voluntary, unsigned long *nonvoluntary, int *threads);
void obter_syscalls(int pid, unsigned long *syscalls_read, unsigned long *syscalls_write);
void obter_prioridade_nice(int pid, int *priority, int *nice_value);
void obter_nome_processo(int pid, char *buffer, size_t size);
int verificar_processo_existe(int pid);

void formatar_taxa(double taxa, char *buffer, size_t size);

#endif