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

void obter_context_switches_e_threads(int pid, unsigned long *voluntary, unsigned long *nonvoluntary, int *threads) {
    char caminho[100];
    FILE *arquivo;
    char linha[256];
    
    *voluntary = 0;
    *nonvoluntary = 0;
    *threads = 0;
    
    sprintf(caminho, "/proc/%d/status", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo) {
        while (fgets(linha, sizeof(linha), arquivo)) {
            if (strstr(linha, "voluntary_ctxt_switches:") == linha) {
                sscanf(linha, "voluntary_ctxt_switches: %lu", voluntary);
            } else if (strstr(linha, "nonvoluntary_ctxt_switches:") == linha) {
                sscanf(linha, "nonvoluntary_ctxt_switches: %lu", nonvoluntary);
            } else if (strstr(linha, "Threads:") == linha) {
                sscanf(linha, "Threads: %d", threads);
            }
        }
        fclose(arquivo);
    }
}

void obter_syscalls_e_iowait(int pid, unsigned long *syscalls_read, unsigned long *syscalls_write, unsigned long *io_wait_ticks) {
    char caminho[100];
    FILE *arquivo;
    char linha[256];
    
    *syscalls_read = 0;
    *syscalls_write = 0;
    *io_wait_ticks = 0;
    
    // Syscalls do /proc/pid/io
    sprintf(caminho, "/proc/%d/io", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo) {
        while (fgets(linha, sizeof(linha), arquivo)) {
            if (strstr(linha, "syscr") == linha) {
                sscanf(linha, "syscr: %lu", syscalls_read);
            } else if (strstr(linha, "syscw") == linha) {
                sscanf(linha, "syscw: %lu", syscalls_write);
            }
        }
        fclose(arquivo);
    }
    
    // I/O wait do /proc/pid/stat (campo 42)
    sprintf(caminho, "/proc/%d/stat", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo) {
        fgets(linha, sizeof(linha), arquivo);
        fclose(arquivo);
        
        char *token = strtok(linha, " ");
        for (int i = 1; i <= 42 && token != NULL; i++) {
            if (i == 42) *io_wait_ticks = strtoul(token, NULL, 10);
            token = strtok(NULL, " ");
        }
    }
}

void obter_prioridade_nice(int pid, int *priority, int *nice_value) {
    char caminho[100];
    FILE *arquivo;
    char linha[512];
    
    *priority = 0;
    *nice_value = 0;
    
    sprintf(caminho, "/proc/%d/stat", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo) {
        fgets(linha, sizeof(linha), arquivo);
        fclose(arquivo);
        
        char *token = strtok(linha, " ");
        for (int i = 1; i <= 19 && token != NULL; i++) {
            if (i == 18) *nice_value = atoi(token);
            else if (i == 19) *priority = atoi(token);
            token = strtok(NULL, " ");
        }
    }
}

void mostrar_metricas_cpu_avancadas(int pid) {
    printf("\n=== METRICAS AVANCADAS DE CPU ===\n");
    
    unsigned long voluntary, nonvoluntary;
    int threads;
    obter_context_switches_e_threads(pid, &voluntary, &nonvoluntary, &threads);
    printf("Context Switches - Voluntary: %lu, Non-voluntary: %lu\n", voluntary, nonvoluntary);
    printf("Threads: %d\n", threads);
    
    unsigned long syscalls_read, syscalls_write, io_wait_ticks;
    obter_syscalls_e_iowait(pid, &syscalls_read, &syscalls_write, &io_wait_ticks);
    printf("Syscalls - Read: %lu, Write: %lu\n", syscalls_read, syscalls_write);
    printf("I/O Wait Time: %.1f seconds\n", io_wait_ticks / (double)sysconf(_SC_CLK_TCK));
    
    int priority, nice_value;
    obter_prioridade_nice(pid, &priority, &nice_value);
    printf("Priority: %d, Nice: %d\n", priority, nice_value);
}