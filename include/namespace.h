#ifndef NAMESPACE_H
#define NAMESPACE_H

void listar_namespaces_processo(int pid);
void comparar_namespaces(const char* pids_str);
void encontrar_processos_no_namespace(int pid_referencia, const char* tipo_ns);
void gerar_relatorio_namespaces_sistema(void);

#endif // NAMESPACE_H