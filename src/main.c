#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/monitor.h"

typedef struct {
    int pid;
    char nome[256];
    double cpu_usage;
    double memoria_mb;
    double io_read_rate;
    double io_write_rate;
    double net_rx_rate;
    double net_tx_rate;
    double net_rx_packets_rate;
    double net_tx_packets_rate;
    IOMetrics last_io;
    NetMetrics last_net;
    unsigned long last_cpu_time;
} ProcessMetrics;

int* parse_pids_argumento(const char *pids_str, int *num_pids) {
    char *copia = strdup(pids_str);
    char *token;
    int *pids = malloc(10 * sizeof(int));
    *num_pids = 0;
    
    token = strtok(copia, ",");
    while (token != NULL && *num_pids < 10) {
        pids[(*num_pids)++] = atoi(token);
        token = strtok(NULL, ",");
    }
    
    free(copia);
    return pids;
}

int verificar_processo_existe(int pid) {
    char caminho[100];
    sprintf(caminho, "/proc/%d/stat", pid);
    FILE *arquivo = fopen(caminho, "r");
    if (arquivo) {
        fclose(arquivo);
        return 1;
    }
    return 0;
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

void formatar_taxa_io(double taxa, char *buffer, size_t size) {
    if (taxa < 1024) {
        snprintf(buffer, size, "%.0fB/s", taxa);
    } else if (taxa < 1024 * 1024) {
        snprintf(buffer, size, "%.1fKB/s", taxa / 1024);
    } else {
        snprintf(buffer, size, "%.1fMB/s", taxa / (1024 * 1024));
    }
}

void mostrar_cabecalho(int num_pids, int intervalo) {
    printf("===========  MONITORANDO %d PROCESSOS (Intervalo: %ds)  ===========\n", num_pids, intervalo);
    printf("PID     NOME           CPU%%    MEMORIA    I/O LEITURA  I/O ESCRITA  REDE RX      REDE TX\n");
    printf("-----------------------------------------------------------------------------------------\n");
}

void atualizar_metricas_processo(ProcessMetrics *metrics, int intervalo) {
    if (!verificar_processo_existe(metrics->pid)) {
        metrics->cpu_usage = 0;
        metrics->memoria_mb = 0;
        metrics->io_read_rate = 0;
        metrics->io_write_rate = 0;
        // Adicione estas linhas:
        metrics->net_rx_rate = 0;
        metrics->net_tx_rate = 0;
        metrics->net_rx_packets_rate = 0;
        metrics->net_tx_packets_rate = 0;
        strcpy(metrics->nome, "TERMINADO");
        return;
    }
    
    metrics->cpu_usage = calcular_cpu_usage_contorno(metrics->pid, &metrics->last_cpu_time);
    metrics->memoria_mb = calcular_uso_memoria_mb(metrics->pid);
    
    IOMetrics io_atual = obter_io_metrics(metrics->pid);
    calcular_taxas_io(&metrics->last_io, &io_atual, intervalo, &metrics->io_read_rate, &metrics->io_write_rate);
    metrics->last_io = io_atual;
    
    // ADICIONE ESTA PARTE PARA REDE:
    NetMetrics net_atual = obter_metricas_rede(metrics->pid);
    calcular_taxas_rede(&metrics->last_net, &net_atual, intervalo, 
                       &metrics->net_rx_rate, &metrics->net_tx_rate,
                       &metrics->net_rx_packets_rate, &metrics->net_tx_packets_rate);
    metrics->last_net = net_atual;
    
    obter_nome_processo(metrics->pid, metrics->nome, sizeof(metrics->nome));
}

void formatar_taxa_rede(double taxa, char *buffer, size_t size) {
    if (taxa < 1024) {
        snprintf(buffer, size, "%.0fB/s", taxa);
    } else if (taxa < 1024 * 1024) {
        snprintf(buffer, size, "%.1fKB/s", taxa / 1024);
    } else {
        snprintf(buffer, size, "%.1fMB/s", taxa / (1024 * 1024));
    }
}

void mostrar_metricas_processo(ProcessMetrics *metrics) {
    char read_str[32], write_str[32];
    // Adicione estas linhas:
    char net_rx_str[32], net_tx_str[32];
    
    formatar_taxa_io(metrics->io_read_rate, read_str, sizeof(read_str));
    formatar_taxa_io(metrics->io_write_rate, write_str, sizeof(write_str));
    // Adicione estas linhas:
    formatar_taxa_rede(metrics->net_rx_rate, net_rx_str, sizeof(net_rx_str));
    formatar_taxa_rede(metrics->net_tx_rate, net_tx_str, sizeof(net_tx_str));
    
    if (strcmp(metrics->nome, "TERMINADO") == 0) {
        printf("%-7d %-14s %-6s   %-9s  %-11s  %-11s  %-11s  %-11s\n", 
               metrics->pid, metrics->nome, "N/A", "N/A", "N/A", "N/A", "N/A", "N/A");
    } else {
        printf("%-7d %-14s %-6.1f   %-6.1fMB   %-11s  %-11s  %-11s  %-11s\n", 
               metrics->pid, metrics->nome, metrics->cpu_usage, 
               metrics->memoria_mb, read_str, write_str, net_rx_str, net_tx_str);
    }
}

void monitorar_multiplos_processos(int *pids, int num_pids, int intervalo) {
    int iteracao = 0;
    ProcessMetrics *metrics = malloc(num_pids * sizeof(ProcessMetrics));
    
    // Inicializar métricas
    for (int i = 0; i < num_pids; i++) {
        metrics[i].pid = pids[i];
        metrics[i].last_io = obter_io_metrics(pids[i]);
        metrics[i].last_cpu_time = 0;
        metrics[i].cpu_usage = 0;
        metrics[i].memoria_mb = 0;
        metrics[i].io_read_rate = 0;
        metrics[i].io_write_rate = 0;
        obter_nome_processo(pids[i], metrics[i].nome, sizeof(metrics[i].nome));
        
        // Primeira leitura para inicializar
        if (verificar_processo_existe(pids[i])) {
            calcular_cpu_usage_contorno(pids[i], &metrics[i].last_cpu_time);
        }
    }
    
    // Aguardar um ciclo antes de começar o loop
    sleep(1);
    
    while (1) {
        system("clear");
        mostrar_cabecalho(num_pids, intervalo);
        
        for (int i = 0; i < num_pids; i++) {
            atualizar_metricas_processo(&metrics[i], intervalo);
            mostrar_metricas_processo(&metrics[i]);
        }
        
        printf("\nIteracao: %d - [Ctrl+C para sair]", ++iteracao);
        fflush(stdout);
        
        sleep(intervalo);
    }
    
    free(metrics);
}

void mostrar_uso_individual(int pid) {
    printf("\n=== ANALISE DETALHADA DO PROCESSO %d ===\n", pid);
    calcular_cpu_usage(pid);
    
    printf("\n=== INFORMACOES DE MEMORIA ===\n");
    double memoria = calcular_uso_memoria_mb(pid);
    printf("Uso de memoria: %.1f MB\n", memoria);
    
    printf("\n=== INFORMACOES DE I/O ===\n");
    IOMetrics io = obter_io_metrics(pid);
    printf("Bytes lidos: %lu\n", io.read_bytes);
    printf("Bytes escritos: %lu\n", io.write_bytes);
    printf("Chars lidos: %lu\n", io.read_chars);
    printf("Chars escritos: %lu\n", io.write_chars);
}

int main(int argc, char *argv[]) {
    int *pids = NULL;
    int num_pids = 0;
    int intervalo = 2;
    int modo_individual = 0;
    int pid_individual = 0;
    
    if (argc < 2) {
        printf("Uso: %s --pids PID1,PID2,... [--intervalo N]\n", argv[0]);
        printf("       %s --pid PID [--detalhes]\n", argv[0]);
        printf("Exemplo: %s --pids 1,1234 --intervalo 3\n", argv[0]);
        printf("         %s --pid 1234 --detalhes\n", argv[0]);
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--pids") == 0 && i + 1 < argc) {
            pids = parse_pids_argumento(argv[i + 1], &num_pids);
        } else if (strcmp(argv[i], "--pid") == 0 && i + 1 < argc) {
            modo_individual = 1;
            pid_individual = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "--intervalo") == 0 && i + 1 < argc) {
            intervalo = atoi(argv[i + 1]);
            if (intervalo < 1) intervalo = 1;
            if (intervalo > 60) intervalo = 60;
        } else if (strcmp(argv[i], "--detalhes") == 0) {
            // Modo detalhado já é o padrão para --pid
        }
    }
    
    if (modo_individual) {
        if (!verificar_processo_existe(pid_individual)) {
            printf("Erro: Processo %d nao encontrado!\n", pid_individual);
            return 1;
        }
        mostrar_uso_individual(pid_individual);
    } else {
        if (pids == NULL || num_pids == 0) {
            printf("Erro: Nenhum PID válido especificado\n");
            printf("Use --pids PID1,PID2,... ou --pid PID\n");
            return 1;
        }
        
        // Verificar se todos os PIDs existem
        for (int i = 0; i < num_pids; i++) {
            if (!verificar_processo_existe(pids[i])) {
                printf("Aviso: Processo %d nao encontrado!\n", pids[i]);
            }
        }
        
        monitorar_multiplos_processos(pids, num_pids, intervalo);
    }
    
    if (pids != NULL) {
        free(pids);
    }
    
    return 0;
}