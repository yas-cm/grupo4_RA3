#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "monitor.h"

NetMetrics obter_metricas_rede(int pid) {
    NetMetrics metrics = {0, 0, 0, 0};
    char caminho[PROC_PATH_MAX];
    
    snprintf(caminho, sizeof(caminho), "/proc/%d/net/dev", pid);
    FILE *arquivo = fopen(caminho, "r");
    if (arquivo == NULL) return metrics;
    
    char linha[PROC_PATH_MAX * 2];
    fgets(linha, sizeof(linha), arquivo);
    fgets(linha, sizeof(linha), arquivo);
    
    while (fgets(linha, sizeof(linha), arquivo)) {
        char iface[64];
        unsigned long r_bytes, r_packets, t_bytes, t_packets;
        
        if (sscanf(linha, " %[^:]: %lu %lu %*u %*u %*u %*u %*u %*u %lu %lu",
                   iface, &r_bytes, &r_packets, &t_bytes, &t_packets) == 5) {
            
            if (strcmp(iface, "lo") != 0) {
                metrics.rx_bytes += r_bytes;
                metrics.packets_rx += r_packets;
                metrics.tx_bytes += t_bytes;
                metrics.packets_tx += t_packets;
            }
        }
    }
    
    fclose(arquivo);
    return metrics;
}

void calcular_taxas_rede(const NetMetrics *antes, const NetMetrics *depois, double intervalo_sec, double *rx_rate, double *tx_rate) {
    if (intervalo_sec > TIME_DIFF_MIN) {
        *rx_rate = (depois->rx_bytes - antes->rx_bytes) / intervalo_sec;
        *tx_rate = (depois->tx_bytes - antes->tx_bytes) / intervalo_sec;
        if (*rx_rate < 0) *rx_rate = 0;
        if (*tx_rate < 0) *tx_rate = 0;
    } else {
        *rx_rate = 0;
        *tx_rate = 0;
    }
}

NetworkConnections obter_conexoes_rede(int pid) {
    NetworkConnections conns = {0, 0, 0, 0};
    char caminho[PROC_PATH_MAX];
    FILE *arquivo;
    char linha[PROC_PATH_MAX * 2];
    
    snprintf(caminho, sizeof(caminho), "/proc/%d/net/tcp", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo) {
        fgets(linha, sizeof(linha), arquivo);
        while (fgets(linha, sizeof(linha), arquivo)) {
            int state;
            if (sscanf(linha, "%*d: %*x:%*x %*x:%*x %x", &state) == 1) {
                conns.tcp_connections++;
                if (state == 10) conns.tcp_listen++;
                else if (state == 1) conns.tcp_established++;
            }
        }
        fclose(arquivo);
    }
    
    snprintf(caminho, sizeof(caminho), "/proc/%d/net/udp", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo) {
        fgets(linha, sizeof(linha), arquivo);
        while (fgets(linha, sizeof(linha), arquivo)) {
            conns.udp_connections++;
        }
        fclose(arquivo);
    }
    
    return conns;
}