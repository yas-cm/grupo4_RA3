#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/monitor.h"

IOMetrics obter_io_metrics(int pid) {
    char caminho[100];
    FILE *arquivo;
    char linha[256];
    IOMetrics metrics = {0, 0, 0, 0};
    
    sprintf(caminho, "/proc/%d/io", pid);
    arquivo = fopen(caminho, "r");
    
    if (arquivo) {
        while (fgets(linha, sizeof(linha), arquivo)) {
            if (strstr(linha, "read_bytes:") == linha) {
                sscanf(linha, "read_bytes: %lu", &metrics.read_bytes);
            } else if (strstr(linha, "write_bytes:") == linha) {
                sscanf(linha, "write_bytes: %lu", &metrics.write_bytes);
            } else if (strstr(linha, "rchar:") == linha) {
                sscanf(linha, "rchar: %lu", &metrics.read_chars);
            } else if (strstr(linha, "wchar:") == linha) {
                sscanf(linha, "wchar: %lu", &metrics.write_chars);
            } else if (strstr(linha, "syscr:") == linha) {
                // Já tratado em cpu_monitor
            } else if (strstr(linha, "syscw:") == linha) {
                // Já tratado em cpu_monitor
            }
        }
        fclose(arquivo);
    }
    
    return metrics;
}

void calcular_taxas_io(IOMetrics *antes, IOMetrics *depois, int intervalo, double *read_rate, double *write_rate) {
    if (intervalo > 0) {
        *read_rate = (depois->read_chars - antes->read_chars) / (double)intervalo;
        *write_rate = (depois->write_chars - antes->write_chars) / (double)intervalo;
    } else {
        *read_rate = *write_rate = 0;
    }
}

void formatar_taxa_io(double taxa, char *buffer, size_t size) {
    if (taxa < 1024) {
        snprintf(buffer, size, "%.0fB/s", taxa);
    } else if (taxa < 1024 * 1024) {
        snprintf(buffer, size, "%.1fKB/s", taxa / 1024);
    } else {
        snprintf(buffer, size, "%.1fMB/s", taxa / (1024 * 1024));
    }
}

void mostrar_metricas_io_avancadas(int pid) {
    printf("\n=== METRICAS AVANCADAS DE I/O ===\n");
    
    IOMetrics io = obter_io_metrics(pid);
    printf("Bytes lidos: %lu (%.2f MB)\n", io.read_bytes, io.read_bytes / (1024.0 * 1024.0));
    printf("Bytes escritos: %lu (%.2f MB)\n", io.write_bytes, io.write_bytes / (1024.0 * 1024.0));
    printf("Chars lidos: %lu\n", io.read_chars);
    printf("Chars escritos: %lu\n", io.write_chars);
    
    unsigned long syscalls_read, syscalls_write, io_wait_ticks;
    obter_syscalls_e_iowait(pid, &syscalls_read, &syscalls_write, &io_wait_ticks);
    printf("Syscalls de leitura: %lu\n", syscalls_read);
    printf("Syscalls de escrita: %lu\n", syscalls_write);
    printf("Tempo de espera I/O: %.2f segundos\n", io_wait_ticks / (double)sysconf(_SC_CLK_TCK));
}