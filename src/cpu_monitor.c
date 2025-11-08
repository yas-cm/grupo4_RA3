#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "../include/monitor.h"

// Uso ATUAL da CPU (como top)
void calcular_cpu_usage_atual(int pid) {
    char caminho[100];
    FILE *arquivo;
    char linha[512];
    char nome_processo[256];
    unsigned long utime1, stime1, utime2, stime2;
    int i;
    long clock_ticks;
    
    clock_ticks = sysconf(_SC_CLK_TCK);
    
    // PRIMEIRA LEITURA
    sprintf(caminho, "/proc/%d/stat", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) {
        printf("ERRO: Processo %d nao encontrado!\n", pid);
        return;
    }
    
    fgets(linha, sizeof(linha), arquivo);
    fclose(arquivo);
    
    // Parsing
    char *token = strtok(linha, " ");
    for (i = 1; i <= 15 && token != NULL; i++) {
        if (i == 2) {
            strcpy(nome_processo, token);
        } else if (i == 14) {
            utime1 = strtoul(token, NULL, 10);
        } else if (i == 15) {
            stime1 = strtoul(token, NULL, 10);
        }
        token = strtok(NULL, " ");
    }
    
    sleep(1);
    
    // SEGUNDA LEITURA
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) {
        printf("ERRO: Processo %d terminou!\n", pid);
        return;
    }
    
    fgets(linha, sizeof(linha), arquivo);
    fclose(arquivo);
    
    token = strtok(linha, " ");
    for (i = 1; i <= 15 && token != NULL; i++) {
        if (i == 14) {
            utime2 = strtoul(token, NULL, 10);
        } else if (i == 15) {
            stime2 = strtoul(token, NULL, 10);
        }
        token = strtok(NULL, " ");
    }
    
    // Cálculo do uso ATUAL
    unsigned long diff = (utime2 + stime2) - (utime1 + stime1);
    double cpu_usage = ((double)diff / clock_ticks) * 100.0;
    
    printf("=== USO ATUAL DE CPU (como top) ===\n");
    printf("Processo: %s (PID: %d)\n", nome_processo, pid);
    printf("Diff CPU time: %lu jiffies\n", diff);
    printf("Uso de CPU (ultimo 1s): %.1f%%\n", cpu_usage);
}

// Uso MÉDIO da CPU (como ps)
void calcular_cpu_usage_medio(int pid) {
    char caminho[100];
    FILE *arquivo;
    char linha[512];
    char nome_processo[256];
    unsigned long utime, stime, starttime;
    long uptime, clock_ticks;
    int i;
    
    clock_ticks = sysconf(_SC_CLK_TCK);
    
    // Ler uptime do sistema
    arquivo = fopen("/proc/uptime", "r");
    if (arquivo) {
        fscanf(arquivo, "%ld", &uptime);
        fclose(arquivo);
    }
    
    // Ler stats do processo
    sprintf(caminho, "/proc/%d/stat", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) {
        printf("ERRO: Processo %d nao encontrado!\n", pid);
        return;
    }
    
    fgets(linha, sizeof(linha), arquivo);
    fclose(arquivo);
    
    // Parsing
    char *token = strtok(linha, " ");
    for (i = 1; i <= 22 && token != NULL; i++) {
        if (i == 2) {
            strcpy(nome_processo, token);
        } else if (i == 14) {
            utime = strtoul(token, NULL, 10);
        } else if (i == 15) {
            stime = strtoul(token, NULL, 10);
        } else if (i == 22) {
            starttime = strtoul(token, NULL, 10);
        }
        token = strtok(NULL, " ");
    }
    
    // Cálculo do PS: (total_cpu_time / Hertz) / seconds_since_start * 100%
    double seconds_since_start = uptime - (starttime / (double)clock_ticks);
    if (seconds_since_start < 0.1) seconds_since_start = 0.1;
    
    unsigned long total_cpu_time = utime + stime;
    double total_cpu_seconds = total_cpu_time / (double)clock_ticks;
    double cpu_usage_medio = (total_cpu_seconds / seconds_since_start) * 100.0;
    
    printf("=== USO MEDIO DE CPU (como ps aux) ===\n");
    printf("Processo: %s (PID: %d)\n", nome_processo, pid);
    printf("Total CPU time: %lu jiffies (%.1f segundos)\n", total_cpu_time, total_cpu_seconds);
    printf("Tempo desde inicio: %.1f segundos\n", seconds_since_start);
    printf("Uso de CPU MEDIO: %.1f%%\n", cpu_usage_medio);
}

void calcular_cpu_usage(int pid) {
    printf("\n");
    calcular_cpu_usage_atual(pid);
    printf("\n");
    calcular_cpu_usage_medio(pid);
    
    // Comparação com ps real
    printf("\n=== COMPARACAO COM PS REAL ===\n");
    char cmd[100];
    sprintf(cmd, "ps -p %d -o pid,pcpu,pmem,comm,time 2>/dev/null", pid);
    system(cmd);
}

// VERSÃO SIMPLIFICADA E FUNCIONAL para monitoramento contínuo
double calcular_cpu_usage_contorno(int pid, unsigned long *last_total_time) {
    char caminho[100];
    FILE *arquivo;
    char linha[512];
    unsigned long utime = 0, stime = 0;
    int i;
    long clock_ticks = sysconf(_SC_CLK_TCK);
    static time_t last_calculation = 0;
    static double last_usage[10000] = {0};
    
    sprintf(caminho, "/proc/%d/stat", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) {
        return 0.0;
    }
    
    fgets(linha, sizeof(linha), arquivo);
    fclose(arquivo);
    
    // Parsing
    char *token = strtok(linha, " ");
    for (i = 1; i <= 15 && token != NULL; i++) {
        if (i == 14) utime = strtoul(token, NULL, 10);
        else if (i == 15) stime = strtoul(token, NULL, 10);
        token = strtok(NULL, " ");
    }
    
    unsigned long total_time = utime + stime;
    time_t current_time = time(NULL);
    
    if (*last_total_time == 0) {
        *last_total_time = total_time;
        last_calculation = current_time;
        return 0.0;
    }
    
    // Só calcular a cada 2 segundos para maior estabilidade
    if (current_time - last_calculation < 2) {
        return last_usage[pid];
    }
    
    double elapsed = difftime(current_time, last_calculation);
    if (elapsed < 1.5) {
        return last_usage[pid];
    }
    
    unsigned long diff = total_time - *last_total_time;
    *last_total_time = total_time;
    last_calculation = current_time;
    
    double cpu_usage = ((double)diff / clock_ticks / elapsed) * 100.0;
    
    // Filtro simples: descartar valores absurdos
    if (cpu_usage < 0 || cpu_usage > 100) {
        return last_usage[pid];
    }
    
    // Suavização simples
    if (last_usage[pid] > 0) {
        cpu_usage = (cpu_usage + last_usage[pid]) / 2.0;
    }
    
    last_usage[pid] = cpu_usage;
    return cpu_usage;
}