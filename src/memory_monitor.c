#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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