#ifndef CGROUP_MANAGER_H
#define CGROUP_MANAGER_H

/* Constantes para cgroups */
#define CGROUP_PATH_SIZE 512
#define CGROUP_BUFFER_SIZE 4096
#define CGROUP_VALUE_SIZE 128
#define CGROUP_PID_SIZE 32

/**
 * @brief Gera um relatório completo de utilização e limites para um cgroup.
 *
 * Lê e exibe métricas de CPU, Memória e I/O (BlkIO) para um grupo específico,
 * comparando o uso atual com os limites configurados.
 *
 * @param nome_grupo O nome do cgroup a ser inspecionado (ex: "meu_app").
 */
void gerar_relatorio_cgroup(const char *nome_grupo);

/**
 * @brief Cria um novo cgroup nos controladores de CPU, Memória e BlkIO.
 *
 * Esta função cria os diretórios necessários em /sys/fs/cgroup para o novo grupo.
 * Requer privilégios de root.
 *
 * @param nome_grupo O nome do cgroup a ser criado (ex: "grupo_teste").
 * @return 0 em caso de sucesso, -1 em caso de erro.
 */
int criar_cgroup(const char *nome_grupo);

/**
 * @brief Remove um cgroup existente.
 *
 * O cgroup deve estar vazio (não conter processos ou subgrupos) para ser removido.
 * Requer privilégios de root.
 *
 * @param nome_grupo O nome do cgroup a ser removido.
 * @return 0 em caso de sucesso, -1 em caso de erro.
 */
int remover_cgroup(const char *nome_grupo);

/**
 * @brief Move um processo para um cgroup específico em todos os controladores.
 *
 * Escreve o PID do processo no arquivo cgroup.procs dos controladores.
 * Requer privilégios de root para mover processos que não pertencem ao usuário.
 *
 * @param pid O PID do processo a ser movido.
 * @param nome_grupo O cgroup de destino.
 * @return 0 em caso de sucesso, -1 em caso de erro.
 */
int mover_processo_para_cgroup(int pid, const char *nome_grupo);

/**
 * @brief Aplica um limite de uso de CPU a um cgroup.
 *
 * Configura a quota de tempo de CPU dentro de um período, efetivamente limitando
 * o percentual de CPU que o grupo pode usar.
 * Ex: para limitar a 50% de uma CPU, use quota=50000 e periodo=100000.
 *
 * @param nome_grupo O cgroup ao qual o limite será aplicado.
 * @param quota_us A quantidade de microssegundos que o cgroup pode executar em um período.
 * @param periodo_us O comprimento do período em microssegundos.
 * @return 0 em caso de sucesso, -1 em caso de erro.
 */
int aplicar_limite_cpu(const char *nome_grupo, long quota_us, long periodo_us);

/**
 * @brief Aplica um limite de uso de memória (RSS) a um cgroup.
 *
 * @param nome_grupo O cgroup ao qual o limite será aplicado.
 * @param limite_bytes O limite máximo de memória em bytes.
 * @return 0 em caso de sucesso, -1 em caso de erro.
 */
int aplicar_limite_memoria(const char *nome_grupo, long long limite_bytes);

/**
 * @brief Aplica um limite de taxa de escrita de I/O para um dispositivo específico.
 *
 * @param nome_grupo O cgroup ao qual o limite será aplicado.
 * @param dispositivo O caminho do dispositivo de bloco (ex: "/dev/sda").
 * @param bps A taxa máxima de escrita em bytes por segundo.
 * @return 0 em caso de sucesso, -1 em caso de erro.
 */
int aplicar_limite_io_escrita(const char *nome_grupo, const char *dispositivo, long long bps);

#endif // CGROUP_MANAGER_H