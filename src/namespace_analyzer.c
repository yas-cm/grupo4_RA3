#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include "namespace.h"

static int get_ns_id(int pid, const char* ns_type, char* buffer, size_t size) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/ns/%s", pid, ns_type);

    ssize_t len = readlink(path, buffer, size - 1);
    if (len == -1) {
        return 0; 
    }

    buffer[len] = '\0';
    return 1;
}

void listar_namespaces_processo(int pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/ns/", pid);

    DIR *dir = opendir(path);
    if (dir == NULL) {
        fprintf(stderr, "Erro: Não foi possível abrir %s. ", path);
        perror(NULL);
        return;
    }

    printf("=== Namespaces do Processo %d ===\n", pid);
    printf("%-10s: %s\n", "TIPO", "IDENTIFICADOR");
    printf("------------------------------------\n");

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_LNK) {
            char ns_id[256];
            if (get_ns_id(pid, entry->d_name, ns_id, sizeof(ns_id))) {
                printf("%-10s: %s\n", entry->d_name, ns_id);
            }
        }
    }

    closedir(dir);
}

void comparar_namespaces(const char* pids_str) {
    char* pids_copy = strdup(pids_str);
    if (pids_copy == NULL) return;

    const char* pid1_str = strtok(pids_copy, ",");
    const char* pid2_str = strtok(NULL, ",");

    if (pid1_str == NULL || pid2_str == NULL) {
        fprintf(stderr, "Erro: Formato inválido. Use --compare-ns PID1,PID2\n");
        free(pids_copy);
        return;
    }

    int pid1 = atoi(pid1_str);
    int pid2 = atoi(pid2_str);
    free(pids_copy);

    const char* ns_types[] = {"cgroup", "ipc", "mnt", "net", "pid", "user", "uts"};
    int num_ns_types = sizeof(ns_types) / sizeof(ns_types[0]);

    printf("=== Comparando Namespaces: PID %d vs PID %d ===\n", pid1, pid2);
    printf("%-8s | %-15s | %s\n", "TIPO", "STATUS", "IDENTIFICADOR(ES)");
    printf("-----------------------------------------------------------------\n");

    for (int i = 0; i < num_ns_types; i++) {
        char id1[256], id2[256];
        int s1 = get_ns_id(pid1, ns_types[i], id1, sizeof(id1));
        int s2 = get_ns_id(pid2, ns_types[i], id2, sizeof(id2));
        
        if (!s1 || !s2) {
             printf("%-8s | ERRO AO LER\n", ns_types[i]);
             continue;
        }

        if (strcmp(id1, id2) == 0) {
            printf("%-8s | COMPARTILHADO   | %s\n", ns_types[i], id1);
        } else {
            printf("%-8s | DIFERENTE       | PID %d: %s\n", ns_types[i], pid1, id1);
            printf("%-8s | %-15s | PID %d: %s\n", "", "", pid2, id2);
        }
    }
}

void encontrar_processos_no_namespace(int pid_referencia, const char* tipo_ns) {
    char id_alvo[256];
    if (!get_ns_id(pid_referencia, tipo_ns, id_alvo, sizeof(id_alvo))) {
        fprintf(stderr, "Erro: Não foi possível obter o namespace '%s' do PID %d. ", tipo_ns, pid_referencia);
        perror(NULL);
        return;
    }

    printf("Procurando processos no namespace '%s' com ID: %s\n", tipo_ns, id_alvo);
    printf("PIDs encontrados:\n");

    DIR *proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("Erro ao abrir /proc");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        int pid_atual = atoi(entry->d_name);
        if (pid_atual > 0) {
            char id_atual[256];
            if (get_ns_id(pid_atual, tipo_ns, id_atual, sizeof(id_atual))) {
                if (strcmp(id_alvo, id_atual) == 0) {
                    printf("- %d\n", pid_atual);
                }
            }
        }
    }
    closedir(proc_dir);
}

typedef struct {
    char ns_id[128];
    int process_count;
} NamespaceStat;

static void add_or_update_stat(NamespaceStat **stats, int *count, int *capacity, const char *id) {
    for (int i = 0; i < *count; i++) {
        if (strcmp((*stats)[i].ns_id, id) == 0) {
            (*stats)[i].process_count++;
            return;
        }
    }

    if (*count == *capacity) {
        *capacity *= 2;
        *stats = realloc(*stats, *capacity * sizeof(NamespaceStat));
        if (*stats == NULL) exit(1);
    }
    
    strncpy((*stats)[*count].ns_id, id, sizeof((*stats)[*count].ns_id) - 1);
    (*stats)[*count].ns_id[sizeof((*stats)[*count].ns_id) - 1] = '\0';
    (*stats)[*count].process_count = 1;
    (*count)++;
}

void gerar_relatorio_namespaces_sistema(void) {
    printf("Gerando relatório de namespaces do sistema...\n");

    const char* ns_types[] = {"cgroup", "ipc", "mnt", "net", "pid", "user", "uts"};
    int num_ns_types = sizeof(ns_types) / sizeof(ns_types[0]);

    for (int i = 0; i < num_ns_types; i++) {
        int count = 0, capacity = 16;
        NamespaceStat *stats = malloc(capacity * sizeof(NamespaceStat));
        if (stats == NULL) exit(1);
        
        DIR *proc_dir = opendir("/proc");
        if (proc_dir == NULL) { free(stats); return; }

        struct dirent *entry;
        while ((entry = readdir(proc_dir)) != NULL) {
            int pid = atoi(entry->d_name);
            if (pid > 0) {
                char id_atual[256];
                if (get_ns_id(pid, ns_types[i], id_atual, sizeof(id_atual))) {
                    add_or_update_stat(&stats, &count, &capacity, id_atual);
                }
            }
        }
        closedir(proc_dir);

        printf("\n--- Relatório para Namespace '%s' ---\n", ns_types[i]);
        for (int j = 0; j < count; j++) {
            printf("  ID: %-25s | Processos: %d\n", stats[j].ns_id, stats[j].process_count);
        }
        free(stats);
    }
}