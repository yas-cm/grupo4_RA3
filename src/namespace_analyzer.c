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

/**
 * TAREFA 3: Encontra todos os processos que compartilham um namespace específico
 * com um processo de referência.
 */
void encontrar_processos_no_namespace(int pid_referencia, const char* tipo_ns) {
    char id_alvo[256];
    if (!obter_id_namespace(pid_referencia, tipo_ns, id_alvo, sizeof(id_alvo))) {
        fprintf(stderr, "Erro: Não foi possível obter o namespace '%s' do PID de referência %d. ", tipo_ns, pid_referencia);
        perror("Motivo");
        return;
    }

    printf("Procurando por processos no namespace '%s' com ID: %s\n", tipo_ns, id_alvo);
    printf("PIDs encontrados:\n");

    DIR *proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("Erro ao abrir /proc");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        int pid_atual = atoi(entry->d_name);
        if (pid_atual > 0) { // Verifica se o nome do diretório é um número (PID)
            char id_atual[256];
            if (obter_id_namespace(pid_atual, tipo_ns, id_atual, sizeof(id_atual))) {
                if (strcmp(id_alvo, id_atual) == 0) {
                    printf("- %d\n", pid_atual);
                }
            }
        }
    }

    closedir(proc_dir);
}

// --- LÓGICA PARA A TAREFA 4 ---

// Estrutura para armazenar as estatísticas de um único namespace
typedef struct {
    char ns_id[128];
    int process_count;
} NamespaceStat;

// Função auxiliar para adicionar ou atualizar uma estatística em um array dinâmico
static void add_or_update_stat(NamespaceStat **stats_array, int *count, const char *id) {
    // Procura se o ID já existe
    for (int i = 0; i < *count; i++) {
        if (strcmp((*stats_array)[i].ns_id, id) == 0) {
            (*stats_array)[i].process_count++;
            return;
        }
    }

    // Se não encontrou, adiciona um novo
    *stats_array = realloc(*stats_array, (*count + 1) * sizeof(NamespaceStat));
    if (*stats_array == NULL) {
        perror("Falha ao realocar memória para estatísticas");
        exit(1);
    }
    
    strncpy((*stats_array)[*count].ns_id, id, sizeof((*stats_array)[*count].ns_id) - 1);
    (*stats_array)[*count].ns_id[sizeof((*stats_array)[*count].ns_id) - 1] = '\0';
    (*stats_array)[*count].process_count = 1;
    (*count)++;
}


/**
 * TAREFA 4: Gera um relatório de todos os namespaces ativos no sistema e
 * conta quantos processos pertencem a cada um.
 */
void gerar_relatorio_namespaces_sistema(void) {
    printf("Gerando relatório de namespaces do sistema... (pode levar um momento)\n");

    const char* ns_types[] = {"cgroup", "ipc", "mnt", "net", "pid", "user", "uts"};
    int num_ns_types = sizeof(ns_types) / sizeof(ns_types[0]);

    // Cria um array de stats para cada tipo de namespace
    NamespaceStat *stats[num_ns_types];
    int counts[num_ns_types];
    for(int i=0; i<num_ns_types; ++i) {
        stats[i] = NULL;
        counts[i] = 0;
    }

    DIR *proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("Erro ao abrir /proc. Execute com sudo.");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        int pid = atoi(entry->d_name);
        if (pid > 0) {
            for (int i = 0; i < num_ns_types; i++) {
                char id_atual[256];
                if (obter_id_namespace(pid, ns_types[i], id_atual, sizeof(id_atual))) {
                    add_or_update_stat(&stats[i], &counts[i], id_atual);
                }
            }
        }
    }
    closedir(proc_dir);

    // Imprime o relatório
    for (int i = 0; i < num_ns_types; i++) {
        printf("\n--- Relatório para Namespace '%s' ---\n", ns_types[i]);
        if (counts[i] == 0) {
            printf("Nenhum namespace encontrado ou acessível.\n");
        }
        for (int j = 0; j < counts[i]; j++) {
            printf("  ID: %-25s | Processos: %d\n", stats[i][j].ns_id, stats[i][j].process_count);
        }
        // Libera a memória alocada para este tipo de namespace
        free(stats[i]);
    }
}