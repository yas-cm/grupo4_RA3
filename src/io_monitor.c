#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    unsigned long read_bytes;
    unsigned long write_bytes;
    unsigned long read_chars;
    unsigned long write_chars;
} IOMetrics;

IOMetrics obter_io_metrics(int pid) {
    char caminho[100];
    FILE *arquivo;
    char linha[256];
    IOMetrics metrics = {0, 0, 0, 0};
    
    sprintf(caminho, "/proc/%d/io", pid);
    arquivo = fopen(caminho, "r");
    
    if (arquivo) {
        while (fgets(linha, sizeof(linha), arquivo)) {
            if (strstr(linha, "read_bytes:") == linha) {
                sscanf(linha, "read_bytes: %lu", &metrics.read_bytes);
            } else if (strstr(linha, "write_bytes:") == linha) {
                sscanf(linha, "write_bytes: %lu", &metrics.write_bytes);
            } else if (strstr(linha, "rchar:") == linha) {
                sscanf(linha, "rchar: %lu", &metrics.read_chars);
            } else if (strstr(linha, "wchar:") == linha) {
                sscanf(linha, "wchar: %lu", &metrics.write_chars);
            }
        }
        fclose(arquivo);
    }
    
    return metrics;
}

void calcular_taxas_io(IOMetrics *antes, IOMetrics *depois, int intervalo, double *read_rate, double *write_rate) {
    if (intervalo > 0) {
        *read_rate = (depois->read_chars - antes->read_chars) / (double)intervalo;
        *write_rate = (depois->write_chars - antes->write_chars) / (double)intervalo;
    } else {
        *read_rate = *write_rate = 0;
    }
}