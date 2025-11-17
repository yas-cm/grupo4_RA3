#ifndef CGROUP_MANAGER_H
#define CGROUP_MANAGER_H

#define CGROUP_PATH_SIZE 512
#define CGROUP_BUFFER_SIZE 4096
#define CGROUP_VALUE_SIZE 128
#define CGROUP_PID_SIZE 32

void gerar_relatorio_cgroup(const char *nome_grupo);
int criar_cgroup(const char *nome_grupo);
int remover_cgroup(const char *nome_grupo);
int mover_processo_para_cgroup(int pid, const char *nome_grupo);

// Limita CPU: ex. quota=50000, periodo=100000 = 50% de uma CPU
int aplicar_limite_cpu(const char *nome_grupo, long quota_us, long periodo_us);

int aplicar_limite_memoria(const char *nome_grupo, long long limite_bytes);
int aplicar_limite_io_escrita(const char *nome_grupo, const char *dispositivo, long long bps);

#endif // CGROUP_MANAGER_H