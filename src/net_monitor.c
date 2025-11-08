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

// Método alternativo simplificado - SEM DT_LNK
NetMetrics obter_metricas_rede_por_sockets(int pid) {
    NetMetrics metrics = {0, 0, 0, 0};
    char caminho[256];
    DIR *dir;
    struct dirent *entry;
    
    // Buscar na pasta /proc/[pid]/fd para encontrar sockets
    sprintf(caminho, "/proc/%d/fd", pid);
    dir = opendir(caminho);
    
    if (dir == NULL) {
        return metrics;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        // Ignorar diretórios . e ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char link_path[512], target[512];
        sprintf(link_path, "/proc/%d/fd/%s", pid, entry->d_name);
        
        ssize_t len = readlink(link_path, target, sizeof(target)-1);
        if (len != -1) {
            target[len] = '\0';
            // Verificar se é um socket pela string do target
            if (strncmp(target, "socket:", 7) == 0) {
                // É um socket - em uma implementação completa,
                // buscaríamos estatísticas específicas do socket
            }
        }
    }
    
    closedir(dir);
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