#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "../include/monitor.h"

NetMetrics obter_metricas_rede(int pid) {
    NetMetrics metrics = {0, 0, 0, 0};
    char caminho[256];
    
    // Buscar na pasta /proc/[pid]/net/dev
    sprintf(caminho, "/proc/%d/net/dev", pid);
    FILE *arquivo = fopen(caminho, "r");
    
    if (arquivo == NULL) {
        // Se não conseguir acessar, retorna métricas zeradas
        return metrics;
    }
    
    char linha[512];
    int linha_num = 0;
    
    // Pular as duas primeiras linhas (cabeçalho)
    while (fgets(linha, sizeof(linha), arquivo) && linha_num < 2) {
        linha_num++;
    }
    
    // Ler estatísticas de cada interface
    while (fgets(linha, sizeof(linha), arquivo)) {
        char interface[64];
        unsigned long rx_bytes, rx_packets, tx_bytes, tx_packets;
        
        // Formato CORRIGIDO: interface: rx_bytes rx_packets ... tx_bytes tx_packets
        if (sscanf(linha, " %[^:]: %lu %lu %*u %*u %*u %*u %*u %*u %lu %lu",
                   interface, &rx_bytes, &rx_packets, &tx_bytes, &tx_packets) == 5) {
            
            // Ignorar interface lo (loopback)
            if (strcmp(interface, "lo") != 0) {
                metrics.rx_bytes += rx_bytes;
                metrics.packets_rx += rx_packets;
                metrics.tx_bytes += tx_bytes;
                metrics.packets_tx += tx_packets;
            }
        }
    }
    
    fclose(arquivo);
    return metrics;
}

void calcular_taxas_rede(NetMetrics *antes, NetMetrics *depois, int intervalo, 
                        double *rx_rate, double *tx_rate, double *rx_packets_rate, double *tx_packets_rate) {
    if (intervalo > 0) {
        *rx_rate = (depois->rx_bytes - antes->rx_bytes) / (double)intervalo;
        *tx_rate = (depois->tx_bytes - antes->tx_bytes) / (double)intervalo;
        *rx_packets_rate = (depois->packets_rx - antes->packets_rx) / (double)intervalo;
        *tx_packets_rate = (depois->packets_tx - antes->packets_tx) / (double)intervalo;
    } else {
        *rx_rate = *tx_rate = *rx_packets_rate = *tx_packets_rate = 0;
    }
}

void formatar_taxa_rede(double taxa, char *buffer, size_t size) {
    if (taxa < 1024) {
        snprintf(buffer, size, "%.0fB/s", taxa);
    } else if (taxa < 1024 * 1024) {
        snprintf(buffer, size, "%.1fKB/s", taxa / 1024);
    } else {
        snprintf(buffer, size, "%.1fMB/s", taxa / (1024 * 1024));
    }
}

NetworkConnections obter_conexoes_rede(int pid) {
    NetworkConnections conns = {0, 0, 0, 0, 0, 0};
    char caminho[256];
    FILE *arquivo;
    char linha[512];
    
    // TCP connections
    sprintf(caminho, "/proc/%d/net/tcp", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo) {
        // Pular cabeçalho
        fgets(linha, sizeof(linha), arquivo);
        
        while (fgets(linha, sizeof(linha), arquivo)) {
            char state[16];
            if (sscanf(linha, "%*d: %*64[0-9A-Fa-f]:%*x %*64[0-9A-Fa-f]:%*x %s", state) == 1) {
                conns.tcp_connections++;
                if (strcmp(state, "0A") == 0) conns.tcp_listen++;
                else if (strcmp(state, "01") == 0) conns.tcp_established++;
                else if (strcmp(state, "08") == 0) conns.tcp_close_wait++;
                else if (strcmp(state, "06") == 0) conns.tcp_time_wait++;
            }
        }
        fclose(arquivo);
    }
    
    // UDP connections (simplificado)
    sprintf(caminho, "/proc/%d/net/udp", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo) {
        // Pular cabeçalho
        fgets(linha, sizeof(linha), arquivo);
        
        while (fgets(linha, sizeof(linha), arquivo)) {
            conns.udp_connections++;
        }
        fclose(arquivo);
    }
    
    return conns;
}

void mostrar_metricas_rede_avancadas(int pid) {
    printf("\n=== METRICAS AVANCADAS DE REDE ===\n");
    
    NetMetrics net = obter_metricas_rede(pid);
    printf("Bytes recebidos: %lu (%.2f MB)\n", net.rx_bytes, net.rx_bytes / (1024.0 * 1024.0));
    printf("Bytes enviados: %lu (%.2f MB)\n", net.tx_bytes, net.tx_bytes / (1024.0 * 1024.0));
    printf("Pacotes recebidos: %lu\n", net.packets_rx);
    printf("Pacotes enviados: %lu\n", net.packets_tx);
    
    NetworkConnections conns = obter_conexoes_rede(pid);
    printf("Conexões TCP: %d\n", conns.tcp_connections);
    printf("  - Listen: %d, Established: %d\n", conns.tcp_listen, conns.tcp_established);
    printf("  - Close Wait: %d, Time Wait: %d\n", conns.tcp_close_wait, conns.tcp_time_wait);
    printf("Conexões UDP: %d\n", conns.udp_connections);
}

