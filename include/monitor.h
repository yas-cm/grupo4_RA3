// Em include/monitor.h (VERSÃO COMPLETA E CORRIGIDA)

#ifndef MONITOR_H
#define MONITOR_H

#include <time.h>

// --- DEFINIÇÕES DE STRUCTS PRIMÁRIAS ---
// Estas devem vir primeiro, pois são usadas por ProcessMetrics.

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

// --- ESTRUTURA PRINCIPAL DE MÉTRICAS ---
// Agora que IOMetrics e NetMetrics são conhecidas, esta struct compila.

typedef struct {
    // --- Identificação ---
    int pid;
    char nome[256];
    
    // --- Métricas Exibidas ---
    // Este é o único campo de 'cpu_usage' que deve existir.
    // Ele armazenará o valor suavizado.
    double cpu_usage;           // %

    double memoria_mb;          // RSS em MB
    double io_read_rate;        // B/s
    double io_write_rate;       // B/s
    double net_rx_rate;         // B/s
    double net_tx_rate;         // B/s
    double net_rx_packets_rate; // pacotes/s
    double net_tx_packets_rate; // pacotes/s

    // --- Métricas Avançadas ---
    int num_threads;
    unsigned long voluntary_ctxt_switches;
    unsigned long nonvoluntary_ctxt_switches;
    unsigned long minor_page_faults;
    unsigned long major_page_faults;
    double swap_mb;
    unsigned long syscalls_read;
    unsigned long syscalls_write;

    // --- Estado para Cálculos Incrementais ---
    IOMetrics last_io;
    NetMetrics last_net;
    unsigned long last_total_jiffies;
    struct timespec last_time_snapshot;

} ProcessMetrics;

// --- Estrutura Auxiliar para Conexões ---
typedef struct {
    int tcp_connections;
    int udp_connections;
    int tcp_listen;
    int tcp_established;
    int tcp_close_wait;
    int tcp_time_wait;
} NetworkConnections;


// --- PROTÓTIPOS DE FUNÇÕES ---
// (O resto do arquivo continua igual)

void calcular_cpu_usage(ProcessMetrics *metrics);
double calcular_uso_memoria_mb(int pid);
unsigned long obter_rss(int pid);
unsigned long obter_vsz(int pid);
void obter_page_faults_e_swap(int pid, unsigned long *minor_faults, unsigned long *major_faults, unsigned long *swap_kb);
IOMetrics obter_io_metrics(int pid);
void calcular_taxas_io(IOMetrics *antes, IOMetrics *depois, int intervalo, double *read_rate, double *write_rate);
NetMetrics obter_metricas_rede(int pid);
void calcular_taxas_rede(NetMetrics *antes, NetMetrics *depois, int intervalo, 
                        double *rx_rate, double *tx_rate, double *rx_packets_rate, double *tx_packets_rate);
NetworkConnections obter_conexoes_rede(int pid);
void obter_context_switches_e_threads(int pid, unsigned long *voluntary, unsigned long *nonvoluntary, int *threads);
void obter_syscalls_e_iowait(int pid, unsigned long *syscalls_read, unsigned long *syscalls_write, unsigned long *io_wait_ticks);
void obter_prioridade_nice(int pid, int *priority, int *nice_value);
void obter_nome_processo(int pid, char *buffer, size_t size);
void formatar_taxa_io(double taxa, char *buffer, size_t size);
void formatar_taxa_rede(double taxa, char *buffer, size_t size);
int verificar_processo_existe(int pid);
void mostrar_metricas_memoria_avancadas(int pid);
void mostrar_metricas_io_avancadas(int pid);
void mostrar_metricas_rede_avancadas(int pid);
void mostrar_metricas_avancadas(int pid);

#endif // MONITOR_H