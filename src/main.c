#include <stdio.h>
#include "../include/monitor.h"

int main() {
    int pid;
    
    printf("=== RESOURCE MONITOR ===\n");
    printf("Digite o PID para monitorar: ");
    scanf("%d", &pid);
    
    calcular_cpu_usage(pid);
    
    return 0;
}