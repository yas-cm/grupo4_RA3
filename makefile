# Nome do executável final
TARGET = resource_monitor

# Compilador
CC = gcc

# Flags de Compilação
# A flag -D_POSIX_C_SOURCE=200809L é a mais importante aqui.
CFLAGS = -D_POSIX_C_SOURCE=200809L -Wall -Wextra -std=c99 -I./include

# Pasta dos fontes e objetos
SRC_DIR = src
OBJ_DIR = obj

# Lista de arquivos fonte (.c)
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Gera a lista de arquivos objeto (.o) a partir da lista de fontes
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Regra principal: compila o executável
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

# Regra para compilar cada arquivo .c em um .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para limpar os arquivos gerados
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean