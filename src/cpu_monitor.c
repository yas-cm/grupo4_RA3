#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "../include/monitor.h"

// Em src/cpu_monitor.c

// Substitua a sua função calcular_cpu_usage existente por esta:

void calcular_cpu_usage(ProcessMetrics *metrics) {
    char caminho[100];
    FILE *arquivo;
    char linha[1024];
    
    unsigned long utime, stime;
    long clock_ticks = sysconf(_SC_CLK_TCK);

    sprintf(caminho, "/proc/%d/stat", metrics->pid);
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) {
        // Se o processo sumiu, o uso de CPU deve zerar gradualmente.
        // Aplicamos a suavização com um valor bruto de 0.
        metrics->cpu_usage = metrics->cpu_usage * 0.7; 
        if (metrics->cpu_usage < 0.1) metrics->cpu_usage = 0.0;
        return;
    }

    if (fgets(linha, sizeof(linha), arquivo) == NULL) {
        fclose(arquivo);
        metrics->cpu_usage = 0.0;
        return;
    }
    fclose(arquivo);

    sscanf(linha, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu", &utime, &stime);

    unsigned long total_jiffies = utime + stime;
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    // Se for a primeira medição (last_total_jiffies == 0),
    // apenas armazenamos os valores e saímos. O uso de CPU continua 0.
    if (metrics->last_total_jiffies == 0) {
        metrics->last_total_jiffies = total_jiffies;
        metrics->last_time_snapshot = current_time;
        // Não definimos cpu_usage aqui, ele começará em 0 e aumentará gradualmente
        return;
    }

    double jiffies_diff = total_jiffies - metrics->last_total_jiffies;
    double time_diff_sec = (current_time.tv_sec - metrics->last_time_snapshot.tv_sec) + 
                           (current_time.tv_nsec - metrics->last_time_snapshot.tv_nsec) / 1e9;

    metrics->last_total_jiffies = total_jiffies;
    metrics->last_time_snapshot = current_time;

    // Evita divisão por zero ou picos absurdos em intervalos muito curtos
    if (time_diff_sec < 0.1) { 
        // Se o intervalo for muito curto, não atualizamos o valor, mantemos o suavizado anterior.
        return;
    }

    // --- CÁLCULO DO USO BRUTO E APLICAÇÃO DA SUAVIZAÇÃO ---

    // 1. Calcula o uso de CPU bruto para este intervalo específico
    double cpu_seconds_used = jiffies_diff / (double)clock_ticks;
    double raw_cpu_usage = (cpu_seconds_used / time_diff_sec) * 100.0;
    
    // Garante que o valor bruto não seja negativo.
    if (raw_cpu_usage < 0.0) raw_cpu_usage = 0.0;
    
    // 2. Aplica a Média Móvel Exponencial (EMA) para suavizar o valor
    // A fórmula dá 70% de "peso" ao valor histórico e 30% ao valor novo.
    // Isso amortece os picos e vales, resultando em um valor mais estável.
    metrics->cpu_usage = (metrics->cpu_usage * 0.7) + (raw_cpu_usage * 0.3);
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
    
    sprintf(caminho, "/proc/%d/stat", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo) {
        // O campo 42 (delayacct_blkio_ticks) representa o tempo de espera de I/O
        fscanf(arquivo, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*u %*u %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %lu", io_wait_ticks);
        fclose(arquivo);
    }
}

void obter_prioridade_nice(int pid, int *priority, int *nice_value) {
    char caminho[100];
    FILE *arquivo;
    
    *priority = 0;
    *nice_value = 0;
    
    sprintf(caminho, "/proc/%d/stat", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo) {
        // Campo 18 (priority) e 19 (nice)
        fscanf(arquivo, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*u %*u %d %d", priority, nice_value);
        fclose(arquivo);
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
    printf("I/O Wait Time: %.2f seconds\n", io_wait_ticks / (double)sysconf(_SC_CLK_TCK));
    
    int priority, nice_value;
    obter_prioridade_nice(pid, &priority, &nice_value);
    printf("Priority: %d, Nice: %d\n", priority, nice_value);
}