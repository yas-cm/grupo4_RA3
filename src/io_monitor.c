#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "monitor.h"

IOMetrics obter_io_metrics(int pid) {
    char caminho[PROC_PATH_MAX];
    FILE *arquivo;
    char linha[PROC_PATH_MAX];
    IOMetrics metrics = {0, 0};
    
    snprintf(caminho, sizeof(caminho), "/proc/%d/io", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) return metrics;
    
    while (fgets(linha, sizeof(linha), arquivo)) {
        if (sscanf(linha, "read_bytes: %lu", &metrics.read_bytes) == 1) continue;
        if (sscanf(linha, "write_bytes: %lu", &metrics.write_bytes) == 1) continue;
    }
    fclose(arquivo);
    
    return metrics;
}

void calcular_taxas_io(const IOMetrics *antes, const IOMetrics *depois, double intervalo_sec, double *read_rate, double *write_rate) {
    if (intervalo_sec > TIME_DIFF_MIN) {
        *read_rate = (depois->read_bytes - antes->read_bytes) / intervalo_sec;  
        *write_rate = (depois->write_bytes - antes->write_bytes) / intervalo_sec; 
        if (*read_rate < 0) *read_rate = 0;
        if (*write_rate < 0) *write_rate = 0;
    } else {
        *read_rate = 0;
        *write_rate = 0;
    }
}

void formatar_taxa(double taxa, char *buffer, size_t size) {
    if (taxa < BYTES_PER_KB) {
        snprintf(buffer, size, "%.0f B/s", taxa);
    } else if (taxa < BYTES_PER_MB) {
        snprintf(buffer, size, "%.1f KB/s", taxa / BYTES_PER_KB);
    } else {
        snprintf(buffer, size, "%.1f MB/s", taxa / BYTES_PER_MB);
    }
}