#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include "../include/monitor.h"

unsigned long obter_rss(int pid) {
    char caminho[100];
    FILE *arquivo;
    unsigned long rss = 0;
    unsigned long dummy;
    
    sprintf(caminho, "/proc/%d/statm", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo) {
        fscanf(arquivo, "%lu %lu", &dummy, &rss);
        fclose(arquivo);
    }
    return rss;
}

unsigned long obter_vsz(int pid) {
    char caminho[100];
    FILE *arquivo;
    unsigned long vsz = 0;
    
    sprintf(caminho, "/proc/%d/statm", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo) {
        fscanf(arquivo, "%lu", &vsz);
        fclose(arquivo);
    }
    return vsz;
}

double calcular_uso_memoria_mb(int pid) {
    unsigned long rss = obter_rss(pid);
    long page_size = sysconf(_SC_PAGESIZE);
    
    return (rss * page_size) / (1024.0 * 1024.0);
}

void obter_page_faults_e_swap(int pid, unsigned long *minor_faults, unsigned long *major_faults, unsigned long *swap_kb) {
    char caminho[100];
    FILE *arquivo;
    char linha[512];
    
    *minor_faults = 0;
    *major_faults = 0;
    *swap_kb = 0;
    
    // Page faults do /proc/pid/stat
    sprintf(caminho, "/proc/%d/stat", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo) {
        fgets(linha, sizeof(linha), arquivo);
        fclose(arquivo);
        
        char *token = strtok(linha, " ");
        for (int i = 1; i <= 15 && token != NULL; i++) {
            if (i == 10) *minor_faults = strtoul(token, NULL, 10);
            else if (i == 12) *major_faults = strtoul(token, NULL, 10);
            token = strtok(NULL, " ");
        }
    }
    
    // Swap do /proc/pid/status
    sprintf(caminho, "/proc/%d/status", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo) {
        while (fgets(linha, sizeof(linha), arquivo)) {
            if (strstr(linha, "VmSwap:") == linha) {
                sscanf(linha, "VmSwap: %lu kB", swap_kb);
            }
        }
        fclose(arquivo);
    }
}

void obter_limites_memoria(int pid, unsigned long *soft_limit, unsigned long *hard_limit) {
    // Remover o warning do parâmetro não usado
    (void)pid;  // Esta linha resolve o warning
    
    struct rlimit rlim;
    
    if (getrlimit(RLIMIT_AS, &rlim) == 0) {
        *soft_limit = rlim.rlim_cur;
        *hard_limit = rlim.rlim_max;
    } else {
        *soft_limit = *hard_limit = 0;
    }
}

void mostrar_metricas_memoria_avancadas(int pid) {
    printf("\n=== METRICAS AVANCADAS DE MEMORIA ===\n");
    
    unsigned long minor_faults, major_faults, swap_kb;
    obter_page_faults_e_swap(pid, &minor_faults, &major_faults, &swap_kb);
    
    unsigned long soft_limit, hard_limit;
    obter_limites_memoria(pid, &soft_limit, &hard_limit);
    
    double memoria_rss = calcular_uso_memoria_mb(pid);
    unsigned long vsz = obter_vsz(pid);
    double memoria_vsz = (vsz * sysconf(_SC_PAGESIZE)) / (1024.0 * 1024.0);
    
    printf("RSS: %.1f MB\n", memoria_rss);
    printf("VSZ: %.1f MB\n", memoria_vsz);
    printf("Page Faults - Minor: %lu, Major: %lu\n", minor_faults, major_faults);
    printf("Swap Usage: %.1f MB\n", swap_kb / 1024.0);
    printf("Limites de Memória - Soft: %.1f MB, Hard: %.1f MB\n", 
           soft_limit / (1024.0 * 1024.0), hard_limit / (1024.0 * 1024.0));
    
    // Adicionar informações de páginas
    printf("Page Size: %ld bytes\n", sysconf(_SC_PAGESIZE));
    printf("Total Pages (RSS): %lu\n", (unsigned long)(memoria_rss * 1024 * 1024 / sysconf(_SC_PAGESIZE)));
}

void obter_nome_processo(int pid, char *buffer, size_t size) {
    char caminho[100];
    FILE *arquivo;
    char linha[512];
    
    sprintf(caminho, "/proc/%d/stat", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) {
        strncpy(buffer, "TERMINADO", size);
        return;
    }
    
    fgets(linha, sizeof(linha), arquivo);
    fclose(arquivo);
    
    char *token = strtok(linha, " ");
    for (int i = 1; i <= 2 && token != NULL; i++) {
        if (i == 2) {
            // Remove parênteses do nome
            char *nome = token;
            if (nome[0] == '(' && nome[strlen(nome)-1] == ')') {
                nome[strlen(nome)-1] = '\0';
                nome++;
            }
            strncpy(buffer, nome, size);
            return;
        }
        token = strtok(NULL, " ");
    }
    
    strncpy(buffer, "DESCONHECIDO", size);
}