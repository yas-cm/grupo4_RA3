#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "monitor.h"

double calcular_uso_memoria_mb(int pid) {
    char caminho[PROC_PATH_MAX];
    FILE *arquivo;
    unsigned long rss_pages = 0;
    
    snprintf(caminho, sizeof(caminho), "/proc/%d/statm", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) return 0.0;
    
    if (fscanf(arquivo, "%*d %lu", &rss_pages) != 1) {
        fclose(arquivo);
        return 0.0;
    }
    fclose(arquivo);
    
    long page_size = sysconf(_SC_PAGESIZE);
    return (rss_pages * page_size) / (double)BYTES_PER_MB;
}

void obter_page_faults_e_swap(int pid, unsigned long *minor_faults, unsigned long *major_faults, unsigned long *swap_kb) {
    char caminho[PROC_PATH_MAX];
    FILE *arquivo;
    char linha[PROC_LINE_MAX];
    
    *minor_faults = 0;
    *major_faults = 0;
    *swap_kb = 0;
    
    snprintf(caminho, sizeof(caminho), "/proc/%d/stat", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo != NULL) {
        if (fgets(linha, sizeof(linha), arquivo) != NULL) {
            char *comm_end = strrchr(linha, ')');
            if (comm_end != NULL) {
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
    char caminho[PROC_PATH_MAX];
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
    char caminho[PROC_PATH_MAX];
    snprintf(caminho, sizeof(caminho), "/proc/%d", pid);
    return access(caminho, F_OK) == 0;
}