#include <stdio.h>
#include <unistd.h>
#include "../include/monitor.h"
#include "../include/memory.h"
#include "../include/io.h"

int main() {
    int meu_pid = getpid();
    printf("Testando com PID: %d\n", meu_pid);
    
    calcular_cpu_usage(meu_pid);
    monitorar_memoria(meu_pid);
    monitorar_io(meu_pid);
    
    return 0;
}
