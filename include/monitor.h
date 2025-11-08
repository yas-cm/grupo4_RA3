#ifndef MONITOR_H
#define MONITOR_H

typedef struct {
    unsigned long read_bytes;
    unsigned long write_bytes;
    unsigned long read_chars;
    unsigned long write_chars;
} IOMetrics;

// Adicione estas structs para rede:
typedef struct {
    unsigned long rx_bytes;
    unsigned long tx_bytes;
    unsigned long packets_rx;
    unsigned long packets_tx;
} NetMetrics;

// Protótipos das funções de rede:
NetMetrics obter_metricas_rede(int pid);
void calcular_taxas_rede(NetMetrics *antes, NetMetrics *depois, int intervalo, 
                        double *rx_rate, double *tx_rate, double *rx_packets_rate, double *tx_packets_rate);

// ... (mantenha o resto do arquivo igual)
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

#endif