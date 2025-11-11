#ifndef MONITOR_H
#define MONITOR_H

#include <time.h>
#include <stddef.h>

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
    char nome[256];
    
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
void calcular_taxas_io(IOMetrics *antes, IOMetrics *depois, double intervalo_sec, double *read_rate, double *write_rate);

NetMetrics obter_metricas_rede(int pid);
void calcular_taxas_rede(NetMetrics *antes, NetMetrics *depois, double intervalo_sec, double *rx_rate, double *tx_rate);

NetworkConnections obter_conexoes_rede(int pid);
void obter_context_switches_e_threads(int pid, unsigned long *voluntary, unsigned long *nonvoluntary, int *threads);
void obter_syscalls(int pid, unsigned long *syscalls_read, unsigned long *syscalls_write);
void obter_prioridade_nice(int pid, int *priority, int *nice_value);
void obter_nome_processo(int pid, char *buffer, size_t size);
int verificar_processo_existe(int pid);

void formatar_taxa(double taxa, char *buffer, size_t size);

#endif