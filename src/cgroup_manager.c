#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <errno.h>
#include "cgroup_manager.h"

#define CGROUP_V2_MOUNT "/sys/fs/cgroup"

// Função auxiliar para escrever em um arquivo de cgroup v2
static int escrever_no_arquivo_cgroup(const char *nome_grupo, const char *arquivo, const char *valor) {
    char caminho[512];
    // Se nome_grupo for NULL ou vazio, opera na raiz (para habilitar controladores)
    if (nome_grupo == NULL || strcmp(nome_grupo, "") == 0) {
        snprintf(caminho, sizeof(caminho), "%s/%s", CGROUP_V2_MOUNT, arquivo);
    } else {
        snprintf(caminho, sizeof(caminho), "%s/%s/%s", CGROUP_V2_MOUNT, nome_grupo, arquivo);
    }

    FILE *f = fopen(caminho, "w");
    if (!f) {
        fprintf(stderr, "Erro ao abrir %s: ", caminho);
        perror(NULL);
        return -1;
    }

    if (fprintf(f, "%s", valor) < 0) {
        fprintf(stderr, "Erro ao escrever em %s: ", caminho);
        perror(NULL);
        fclose(f);
        return -1;
    }
    
    fclose(f);
    return 0;
}

// Função auxiliar para ler uma string de um arquivo de cgroup
static char* ler_string_cgroup(const char *nome_grupo, const char *arquivo) {
    char caminho[512];
    snprintf(caminho, sizeof(caminho), "%s/%s/%s", CGROUP_V2_MOUNT, nome_grupo, arquivo);

    FILE *f = fopen(caminho, "r");
    if (!f) return NULL;

    char *buffer = malloc(4096);
    if (!buffer) { fclose(f); return NULL; }
    
    size_t bytes_lidos = fread(buffer, 1, 4095, f);
    buffer[bytes_lidos] = '\0';
    
    fclose(f);
    return buffer;
}


int criar_cgroup(const char *nome_grupo) {
    char caminho[512];
    snprintf(caminho, sizeof(caminho), "%s/%s", CGROUP_V2_MOUNT, nome_grupo);

    struct stat st;
    if (stat(caminho, &st) == 0 && S_ISDIR(st.st_mode)) {
        printf("Aviso: Cgroup '%s' já existe.\n", nome_grupo);
        return 0;
    }

    printf("Criando cgroup v2 '%s'...\n", nome_grupo);
    if (mkdir(caminho, 0755) != 0) {
        fprintf(stderr, "Erro ao criar diretório %s: ", caminho);
        perror(NULL);
        return -1;
    }

    // Em cgroups v2, precisamos habilitar os controladores para o subgrupo a partir do pai.
    if (escrever_no_arquivo_cgroup(NULL, "cgroup.subtree_control", "+cpu +memory +io") != 0) {
        // Isso pode falhar se já estiverem habilitados, o que não é um erro crítico.
        fprintf(stderr, "Aviso: Falha ao habilitar controladores na raiz. Podem já estar habilitados.\n");
    }

    printf("Cgroup '%s' criado com sucesso.\n", nome_grupo);
    return 0;
}

int remover_cgroup(const char *nome_grupo) {
    char caminho[512];
    snprintf(caminho, sizeof(caminho), "%s/%s", CGROUP_V2_MOUNT, nome_grupo);

    printf("Removendo cgroup '%s'...\n", nome_grupo);
    if (rmdir(caminho) != 0) {
        fprintf(stderr, "Erro ao remover o cgroup '%s': ", nome_grupo);
        perror(NULL);
        fprintf(stderr, "Certifique-se de que o cgroup não contém processos e está vazio.\n");
        return -1;
    }
    
    printf("Cgroup '%s' removido com sucesso.\n", nome_grupo);
    return 0;
}

int mover_processo_para_cgroup(int pid, const char *nome_grupo) {
    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d", pid);

    printf("Movendo PID %d para o cgroup v2 '%s'...\n", pid, nome_grupo);
    if (escrever_no_arquivo_cgroup(nome_grupo, "cgroup.procs", pid_str) != 0) {
        fprintf(stderr, "Falha ao mover PID.\n");
        return -1;
    }

    printf("PID %d movido com sucesso.\n", pid);
    return 0;
}

int aplicar_limite_cpu(const char *nome_grupo, long quota_us, long periodo_us) {
    char valor_str[128];
    snprintf(valor_str, sizeof(valor_str), "%ld %ld", quota_us, periodo_us);
    
    if (escrever_no_arquivo_cgroup(nome_grupo, "cpu.max", valor_str) != 0) return -1;
    
    printf("Limite de CPU aplicado ao grupo '%s': %ldus a cada %ldus.\n", nome_grupo, quota_us, periodo_us);
    return 0;
}

int aplicar_limite_memoria(const char *nome_grupo, long long limite_bytes) {
    char valor_str[64];
    snprintf(valor_str, sizeof(valor_str), "%lld", limite_bytes);
    
    if (escrever_no_arquivo_cgroup(nome_grupo, "memory.max", valor_str) != 0) return -1;

    printf("Limite de memória de %lld bytes aplicado ao grupo '%s'.\n", limite_bytes, nome_grupo);
    return 0;
}

int aplicar_limite_io_escrita(const char *nome_grupo, const char *dispositivo, long long bps) {
    struct stat stat_buf;
    if (stat(dispositivo, &stat_buf) != 0) {
        perror("Erro ao obter informações do dispositivo");
        return -1;
    }

    if (!S_ISBLK(stat_buf.st_mode)) {
        fprintf(stderr, "%s não é um dispositivo de bloco.\n", dispositivo);
        return -1;
    }

    unsigned int major_dev = major(stat_buf.st_rdev);
    unsigned int minor_dev = minor(stat_buf.st_rdev);
    
    char valor_str[128];
    snprintf(valor_str, sizeof(valor_str), "%u:%u wbps=%lld", major_dev, minor_dev, bps);

    if (escrever_no_arquivo_cgroup(nome_grupo, "io.max", valor_str) != 0) return -1;
    
    printf("Limite de escrita de I/O de %lld B/s para o dispositivo %u:%u aplicado ao grupo '%s'.\n", bps, major_dev, minor_dev, nome_grupo);
    return 0;
}

void gerar_relatorio_cgroup(const char *nome_grupo) {
    printf("--- Relatório do Cgroup v2: %s ---\n\n", nome_grupo);
    
    // --- CPU ---
    printf("[CPU]\n");
    char* cpu_stat_str = ler_string_cgroup(nome_grupo, "cpu.stat");
    if (cpu_stat_str) {
        long long usage_usec = 0, throttled_usec = 0;
        int nr_throttled = 0;
        char* linha = strtok(cpu_stat_str, "\n");
        while (linha) {
            sscanf(linha, "usage_usec %lld", &usage_usec);
            sscanf(linha, "nr_throttled %d", &nr_throttled);
            sscanf(linha, "throttled_usec %lld", &throttled_usec);
            linha = strtok(NULL, "\n");
        }
        printf("    - Uso Total: %.3f segundos\n", usage_usec / 1e6);
        printf("    - Ocorrências de Throttling: %d\n", nr_throttled);
        printf("    - Tempo Total em Throttling: %.3f segundos\n", throttled_usec / 1e6);
        free(cpu_stat_str);
    } else {
        printf("  Não foi possível ler estatísticas de CPU.\n");
    }
    char* cpu_max_str = ler_string_cgroup(nome_grupo, "cpu.max");
    printf("  Limite Configurado: %s", cpu_max_str ? cpu_max_str : "Ilimitado\n");
    if (cpu_max_str) free(cpu_max_str);

    // --- Memória ---
    printf("\n[Memória]\n");
    char* mem_current_str = ler_string_cgroup(nome_grupo, "memory.current");
    long long mem_current = mem_current_str ? atoll(mem_current_str) : 0;
    if (mem_current_str) free(mem_current_str);

    char* mem_max_str = ler_string_cgroup(nome_grupo, "memory.max");
    long long mem_max = (mem_max_str && strcmp(mem_max_str, "max\n") != 0) ? atoll(mem_max_str) : -1;
    if (mem_max_str) free(mem_max_str);
    
    printf("  Uso Atual: %.2f MB\n", mem_current / (1024.0 * 1024.0));
    if (mem_max < 0) {
        printf("  Limite: Ilimitado\n");
    } else {
        printf("  Limite: %.2f MB\n", mem_max / (1024.0 * 1024.0));
        if (mem_max > 0)
            printf("  Utilização: %.2f%%\n", (double)mem_current / mem_max * 100.0);
    }
    
    char* mem_events_str = ler_string_cgroup(nome_grupo, "memory.events");
    if (mem_events_str) {
        long long oom_count = 0;
        char* linha = strstr(mem_events_str, "oom ");
        if (linha) sscanf(linha, "oom %lld", &oom_count);
        printf("  Eventos de OOM: %lld\n", oom_count);
        free(mem_events_str);
    }

    // --- I/O ---
    printf("\n[I/O]\n");
    char *io_stat_str = ler_string_cgroup(nome_grupo, "io.stat");
    if (io_stat_str && strlen(io_stat_str) > 0) {
        printf("  Estatísticas de I/O:\n%s", io_stat_str);
    } else {
        printf("  Nenhuma estatística de I/O disponível.\n");
    }
    if (io_stat_str) free(io_stat_str);

    printf("\n-----------------------------------\n");
}