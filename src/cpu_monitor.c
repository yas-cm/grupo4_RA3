#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "monitor.h"

void calcular_cpu_usage(ProcessMetrics *metrics) {
    char caminho[256];
    FILE *arquivo;
    unsigned long utime, stime;
    long clock_ticks = sysconf(_SC_CLK_TCK);

    sprintf(caminho, "/proc/%d/stat", metrics->pid);
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) {
        // Se o processo sumiu, aplicamos um decaimento suave no valor de CPU
        metrics->cpu_usage = metrics->cpu_usage * 0.7; 
        if (metrics->cpu_usage < 0.1) metrics->cpu_usage = 0.0;
        return;
    }

    // --- LÓGICA DE LEITURA CORRIGIDA E ROBUSTA ---
    char linha[1024];
    if (fgets(linha, sizeof(linha), arquivo) == NULL) {
        fclose(arquivo);
        metrics->cpu_usage = 0.0;
        return;
    }
    fclose(arquivo);

    // Encontra o fim do nome do processo, que está entre parênteses
    char *comm_end = strrchr(linha, ')');
    if (comm_end == NULL) {
        metrics->cpu_usage = 0.0;
        return;
    }

    // Os campos que queremos (utime e stime) estão após o nome do processo.
    // sscanf começa a ler a partir de 'comm_end + 2' para pular o ')' e o espaço.
    // Os campos são: (14) utime e (15) stime. Pulamos os 11 campos intermediários.
    int result = sscanf(comm_end + 2, 
        "%*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu", 
        &utime, &stime);

    if (result != 2) {
        metrics->cpu_usage = 0.0;
        return;
    }
    // --- FIM DA LÓGICA CORRIGIDA ---

    unsigned long total_jiffies = utime + stime;
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    // Se for a primeira leitura, apenas armazenamos os valores e saímos.
    if (metrics->last_total_jiffies == 0) {
        metrics->last_total_jiffies = total_jiffies;
        metrics->last_time_snapshot = current_time;
        metrics->cpu_usage = 0.0; // Garante que a primeira exibição seja 0
        return;
    }

    double jiffies_diff = total_jiffies - metrics->last_total_jiffies;
    double time_diff_sec = (current_time.tv_sec - metrics->last_time_snapshot.tv_sec) + 
                           (current_time.tv_nsec - metrics->last_time_snapshot.tv_nsec) / 1e9;

    metrics->last_total_jiffies = total_jiffies;
    metrics->last_time_snapshot = current_time;

    if (time_diff_sec < 0.01) { 
        // Intervalo muito curto, mantém o valor anterior para evitar picos estranhos
        return;
    }

    double cpu_seconds_used = jiffies_diff / (double)clock_ticks;
    double raw_cpu_usage = (cpu_seconds_used / time_diff_sec) * 100.0;
    
    if (raw_cpu_usage < 0.0) raw_cpu_usage = 0.0;
    
    // Aplica uma Média Móvel Exponencial para suavizar a saída
    metrics->cpu_usage = (metrics->cpu_usage * 0.7) + (raw_cpu_usage * 0.3);
}

void obter_context_switches_e_threads(int pid, unsigned long *voluntary, unsigned long *nonvoluntary, int *threads) {
    char caminho[256];
    FILE *arquivo;
    char linha[256];
    
    *voluntary = 0;
    *nonvoluntary = 0;
    *threads = 0;
    
    sprintf(caminho, "/proc/%d/status", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) return;

    while (fgets(linha, sizeof(linha), arquivo)) {
        if (sscanf(linha, "voluntary_ctxt_switches: %lu", voluntary) == 1) continue;
        if (sscanf(linha, "nonvoluntary_ctxt_switches: %lu", nonvoluntary) == 1) continue;
        if (sscanf(linha, "Threads: %d", threads) == 1) continue;
    }
    fclose(arquivo);
}

void obter_syscalls(int pid, unsigned long *syscalls_read, unsigned long *syscalls_write) {
    char caminho[256];
    FILE *arquivo;
    char linha[256];
    
    *syscalls_read = 0;
    *syscalls_write = 0;
    
    sprintf(caminho, "/proc/%d/io", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) return;

    while (fgets(linha, sizeof(linha), arquivo)) {
        if (sscanf(linha, "syscr: %lu", syscalls_read) == 1) continue;
        if (sscanf(linha, "syscw: %lu", syscalls_write) == 1) continue;
    }
    fclose(arquivo);
}

void obter_prioridade_nice(int pid, int *priority, int *nice_value) {
    char caminho[256];
    FILE *arquivo;
    
    *priority = 0;
    *nice_value = 0;
    
    sprintf(caminho, "/proc/%d/stat", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) return;

    // Esta leitura também pode ser frágil, mas é menos crítica
    fscanf(arquivo, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*u %*u %d %d", priority, nice_value);
    fclose(arquivo);
}