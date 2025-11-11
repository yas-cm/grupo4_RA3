#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include "../include/monitor.h"

/**
 * @brief 
 * 
 * @param pid 
 * @param ns_type 
 * @param buffer
 * @param size
 * @return
 */
static int obter_id_namespace(int pid, const char* ns_type, char* buffer, size_t size) {
    char path[256];
    sprintf(path, "/proc/%d/ns/%s", pid, ns_type);

    ssize_t len = readlink(path, buffer, size - 1);
    if (len == -1) {
        return 0; 
    }

    buffer[len] = '\0'; // readlink não adiciona o terminador nulo
    return 1;
}

/**
 * TAREFA 1: Lista todos os namespaces de um processo.
 */
void listar_namespaces_processo(int pid) {
    char path[256];
    sprintf(path, "/proc/%d/ns/", pid);

    DIR *dir = opendir(path);
    if (dir == NULL) {
        fprintf(stderr, "Erro: Não foi possível abrir o diretório de namespaces para o PID %d. ", pid);
        perror("Motivo");
        return;
    }

    printf("=== Namespaces do Processo %d ===\n", pid);
    printf("%-18s: %s\n", "TIPO", "IDENTIFICADOR");
    printf("------------------------------------------\n");

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_LNK) {
            char ns_id[256];
            if (obter_id_namespace(pid, entry->d_name, ns_id, sizeof(ns_id))) {
                printf("%-10s: %s\n", entry->d_name, ns_id);
            }
        }
    }

    closedir(dir);
}

/**
 * TAREFA 2: Compara os namespaces entre dois processos.
 */
void comparar_namespaces(const char* pids_str) {
    char* pids_copy = strdup(pids_str); // strdup aloca memória, precisa de free()
    if (pids_copy == NULL) {
        perror("Falha ao alocar memória para PIDs");
        return;
    }

    const char* pid1_str = strtok(pids_copy, ",");
    const char* pid2_str = strtok(NULL, ",");

    if (pid1_str == NULL || pid2_str == NULL) {
        fprintf(stderr, "Erro: Formato inválido. Use --compare-ns PID1,PID2\n");
        free(pids_copy);
        return;
    }

    int pid1 = atoi(pid1_str);
    int pid2 = atoi(pid2_str);

    if (pid1 <= 0 || pid2 <= 0) {
        fprintf(stderr, "Erro: PIDs devem ser números positivos.\n");
        free(pids_copy);
        return;
    }

    const char* ns_types[] = {"cgroup", "ipc", "mnt", "net", "pid", "user", "uts"};
    int num_ns_types = sizeof(ns_types) / sizeof(ns_types[0]);

    printf("=== Comparando Namespaces: PID %d vs PID %d ===\n", pid1, pid2);
    printf("%-8s | %-15s | %s\n", "TIPO", "STATUS", "IDENTIFICADOR(ES)");
    printf("--------------------------------------------------------------------------\n");

    for (int i = 0; i < num_ns_types; i++) {
        char id1[256], id2[256];
        int success1 = obter_id_namespace(pid1, ns_types[i], id1, sizeof(id1));
        int success2 = obter_id_namespace(pid2, ns_types[i], id2, sizeof(id2));
        
        if (!success1 || !success2) {
             printf("%-8s | ERRO AO LER\n", ns_types[i]);
             continue;
        }

        if (strcmp(id1, id2) == 0) {
            printf("%-8s | COMPARTILHADO   | %s\n", ns_types[i], id1);
        } else {
            printf("%-8s | DIFERENTE       | %s (PID %d) vs %s (PID %d)\n", ns_types[i], id1, pid1, id2, pid2);
        }
    }

    free(pids_copy);
}