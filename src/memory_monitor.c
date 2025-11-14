#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "monitor.h"

double calcular_uso_memoria_mb(int pid) {
    char caminho[256];
    FILE *arquivo;
    unsigned long rss_pages = 0;
    
    snprintf(caminho, sizeof(caminho), "/proc/%d/statm", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) return 0.0;
    
    // fscanf aqui é relativamente seguro pois o formato de statm é simples
    if (fscanf(arquivo, "%*d %lu", &rss_pages) != 1) {
        fclose(arquivo);
        return 0.0;
    }
    fclose(arquivo);
    
    long page_size = sysconf(_SC_PAGESIZE);
    return (rss_pages * page_size) / (1024.0 * 1024.0);
}

void obter_page_faults_e_swap(int pid, unsigned long *minor_faults, unsigned long *major_faults, unsigned long *swap_kb) {
    char caminho[256];
    FILE *arquivo;
    char linha[1024];
    
    *minor_faults = 0;
    *major_faults = 0;
    *swap_kb = 0;
    
    snprintf(caminho, sizeof(caminho), "/proc/%d/stat", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo != NULL) {
        if (fgets(linha, sizeof(linha), arquivo) != NULL) {
            char *comm_end = strrchr(linha, ')');
            if (comm_end != NULL) {
                // Pula 7 campos após o nome do processo para chegar em minflt (10) e majflt (12)
                sscanf(comm_end + 2, "%*c %*d %*d %*d %*d %*d %*u %lu %*u %lu",
                       minor_faults, major_faults);
            }
        }
        fclose(arquivo);
    }
    
    snprintf(caminho, sizeof(caminho), "/proc/%d/status", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo != NULL) {
        while (fgets(linha, sizeof(linha), arquivo)) {
            if (sscanf(linha, "VmSwap: %lu kB", swap_kb) == 1) {
                break;
            }
        }
        fclose(arquivo);
    }
}

void obter_nome_processo(int pid, char *buffer, size_t size) {
    char caminho[256];
    FILE *arquivo;
    
    snprintf(caminho, sizeof(caminho), "/proc/%d/comm", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) {
        strncpy(buffer, "N/A", size - 1);
        buffer[size - 1] = '\0';
        return;
    }
    
    if (fgets(buffer, size, arquivo) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0;
    } else {
        strncpy(buffer, "N/A", size - 1);
        buffer[size - 1] = '\0';
    }
    fclose(arquivo);
}

int verificar_processo_existe(int pid) {
    char caminho[256];
    snprintf(caminho, sizeof(caminho), "/proc/%d", pid);
    return access(caminho, F_OK) == 0;
}