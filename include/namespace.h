#ifndef NAMESPACE_H
#define NAMESPACE_H

#define NS_BUFFER_SIZE 256
#define NUM_NS_TYPES 7

void listar_namespaces_processo(int pid);
void comparar_namespaces(const char* pids_str);
void encontrar_processos_no_namespace(int pid_referencia, const char* tipo_ns);
void gerar_relatorio_namespaces_sistema(void);

#endif // NAMESPACE_H