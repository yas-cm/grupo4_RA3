#ifndef MONITOR_H
#define MONITOR_H

#include <time.h>

typedef struct {
    unsigned long read_bytes;
    unsigned long write_bytes;
    unsigned long read_chars;
    unsigned long write_chars;
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
    double net_rx_packets_rate;
    double net_tx_packets_rate;
    IOMetrics last_io;
    NetMetrics last_net;
    unsigned long last_cpu_time;
    int num_threads;
    unsigned long voluntary_ctxt;
    unsigned long nonvoluntary_ctxt;
} ProcessMetrics;

// Estrutura para conexões de rede
typedef struct {
    int tcp_connections;
    int udp_connections;
    int tcp_listen;
    int tcp_established;
    int tcp_close_wait;
    int tcp_time_wait;
} NetworkConnections;

// Funções principais
NetMetrics obter_metricas_rede(int pid);
void calcular_taxas_rede(NetMetrics *antes, NetMetrics *depois, int intervalo, 
                        double *rx_rate, double *tx_rate, double *rx_packets_rate, double *tx_packets_rate);

void calcular_cpu_usage(int pid);
void calcular_cpu_usage_atual(int pid);
void calcular_cpu_usage_medio(int pid);
double calcular_cpu_usage_contorno(int pid, unsigned long *last_total_time);

int* parse_pids_argumento(const char *pids_str, int *num_pids);
int verificar_processo_existe(int pid);
void monitorar_multiplos_processos(int *pids, int num_pids, int intervalo);

double calcular_uso_memoria_mb(int pid);
unsigned long obter_rss(int pid);
unsigned long obter_vsz(int pid);

IOMetrics obter_io_metrics(int pid);
void calcular_taxas_io(IOMetrics *antes, IOMetrics *depois, int intervalo, double *read_rate, double *write_rate);

void obter_nome_processo(int pid, char *buffer, size_t size);
void formatar_taxa_io(double taxa, char *buffer, size_t size);
void formatar_taxa_rede(double taxa, char *buffer, size_t size);
void mostrar_cabecalho(int num_pids, int intervalo);
void atualizar_metricas_processo(ProcessMetrics *metrics, int intervalo);
void mostrar_metricas_processo(ProcessMetrics *metrics);
void mostrar_uso_individual(int pid);

// Novas funções avançadas
void obter_context_switches_e_threads(int pid, unsigned long *voluntary, unsigned long *nonvoluntary, int *threads);
void obter_page_faults_e_swap(int pid, unsigned long *minor_faults, unsigned long *major_faults, unsigned long *swap_kb);
void obter_syscalls_e_iowait(int pid, unsigned long *syscalls_read, unsigned long *syscalls_write, unsigned long *io_wait_ticks);
void obter_prioridade_nice(int pid, int *priority, int *nice_value);
void mostrar_metricas_avancadas(int pid);
void exportar_para_csv(ProcessMetrics *metrics, int num_pids, const char *filename);
NetworkConnections obter_conexoes_rede(int pid);

#endif