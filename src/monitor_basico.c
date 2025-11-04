#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Função para ler tempo total do sistema
unsigned long get_total_system_time() {
    FILE *arquivo = fopen("/proc/stat", "r");
    unsigned long user, nice, system, idle, iowait, irq, softirq;
    
    fscanf(arquivo, "cpu %lu %lu %lu %lu %lu %lu %lu", 
           &user, &nice, &system, &idle, &iowait, &irq, &softirq);
    fclose(arquivo);
    
    return user + nice + system + idle + iowait + irq + softirq;
}

void calcular_cpu_usage(int pid) {
    char caminho[100];
    FILE *arquivo;
    char nome_processo[256];
    char estado;
    unsigned long utime1, stime1, utime2, stime2;
    unsigned long total_time1, total_time2;
    unsigned long system_time1, system_time2;
    unsigned int dummy_uint;
    unsigned long dummy_ulong;
    int num_cores;
    
    // Pega número de núcleos
    num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    
    // PRIMEIRA LEITURA
    sprintf(caminho, "/proc/%d/stat", pid);
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) {
        printf("ERRO: Processo %d nao encontrado!\n", pid);
        return;
    }
    
    fscanf(arquivo, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu",
           &dummy_uint, nome_processo, &estado, &dummy_uint, &dummy_uint,
           &dummy_uint, &dummy_uint, &dummy_uint, &dummy_uint, &dummy_ulong,
           &dummy_ulong, &dummy_ulong, &dummy_ulong, &utime1, &stime1);
    fclose(arquivo);
    
    total_time1 = utime1 + stime1;
    system_time1 = get_total_system_time();
    
    // ESPERA 1 SEGUNDO
    sleep(1);
    
    // SEGUNDA LEITURA
    arquivo = fopen(caminho, "r");
    fscanf(arquivo, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu",
           &dummy_uint, nome_processo, &estado, &dummy_uint, &dummy_uint,
           &dummy_uint, &dummy_uint, &dummy_uint, &dummy_uint, &dummy_ulong,
           &dummy_ulong, &dummy_ulong, &dummy_ulong, &utime2, &stime2);
    fclose(arquivo);
    
    total_time2 = utime2 + stime2;
    system_time2 = get_total_system_time();
    
    // CALCULA USO DE CPU ESTILO PS AUX
    unsigned long process_delta = total_time2 - total_time1;
    unsigned long system_delta = system_time2 - system_time1;

    // DEBUG - mostra os valores brutos
    printf("DEBUG: process_delta = %lu\n", process_delta);
    printf("DEBUG: system_delta = %lu\n", system_delta);
    printf("DEBUG: num_cores = %d\n", num_cores);

    float cpu_percent = 0.0;
    if (system_delta > 0) {
        cpu_percent = (process_delta * 100.0 * num_cores) / system_delta;
    }
    
    printf("=== MONITOR DE CPU ===\n");
    printf("Processo: %s (PID: %d)\n", nome_processo, pid);
    printf("Nucleos disponiveis: %d\n", num_cores);
    printf("Uso de CPU: %.1f%%\n", cpu_percent);
}

int main() {
    int pid;
    
    printf("Digite o PID para monitorar CPU: ");
    scanf("%d", &pid);
    
    calcular_cpu_usage(pid);
    
    return 0;
}