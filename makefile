# Nome do executável final
TARGET = resource_monitor

# Compilador a ser usado
CC = gcc

# Pasta dos fontes, objetos e includes
SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include

# --- FLAGS ---

# Flags de Compilação:
# -D_POSIX_C_SOURCE=200809L: Garante a disponibilidade de funções POSIX como clock_gettime.
# -Wall -Wextra: Ativa todos os warnings importantes. Essencial para código de qualidade.
# -std=c99: Define o padrão da linguagem C.
# -g: Adiciona símbolos de depuração ao executável. CRUCIAL para Valgrind e GDB.
# -I$(INC_DIR): Diz ao compilador onde procurar pelos arquivos de cabeçalho (.h).
CFLAGS = -D_POSIX_C_SOURCE=200809L -Wall -Wextra -std=c99 -g -I$(INC_DIR)

# Flags de Linker:
# -lrt: "Linka" a biblioteca de tempo real (librt), necessária para clock_gettime.
LDFLAGS = -lrt

# --- ARQUIVOS ---

# Lista de arquivos fonte (.c) encontrados automaticamente na pasta src
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Gera a lista de arquivos objeto (.o) a partir da lista de fontes,
# trocando o diretório 'src' por 'obj'.
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))


# --- REGRAS ---

# Regra principal (padrão): compila o executável
all: $(TARGET)

# Regra para linkar todos os arquivos objeto (.o) e criar o executável final
# Esta regra depende que todos os OBJS já tenham sido compilados.
$(TARGET): $(OBJS)
	@echo "Linkando para criar o executável final..."
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Executável '$(TARGET)' criado com sucesso!"

# Regra para compilar cada arquivo .c em um arquivo .o
# $< é o nome da primeira dependência (o arquivo .c)
# $@ é o nome do alvo (o arquivo .o)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@echo "Compilando $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para limpar os arquivos gerados (objetos e o executável)
clean:
	@echo "Limpando arquivos gerados..."
	rm -rf $(OBJ_DIR) $(TARGET)
	@echo "Limpeza concluída."

# Marca as regras que não geram arquivos com o mesmo nome
.PHONY: all clean