TARGET = resource_monitor

CC = gcc

SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include

CFLAGS = -D_POSIX_C_SOURCE=200809L -Wall -Wextra -std=c99 -g -I$(INC_DIR)

LDFLAGS = -lrt

SRCS = $(wildcard $(SRC_DIR)/*.c)

OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "✓ Compilação concluída com sucesso!"
	
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	@echo "✓ Limpeza concluída."

# Target para verificar vazamentos de memória com Valgrind
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
	         --verbose ./$(TARGET) --pids $$(pgrep -n bash) --intervalo 2

# Target para análise estática com cppcheck (se disponível)
check:
	@command -v cppcheck >/dev/null 2>&1 && \
	cppcheck --enable=all --suppress=missingIncludeSystem $(SRC_DIR)/ || \
	echo "cppcheck não disponível"

# Target para formatar código (se clang-format disponível)
format:
	@command -v clang-format >/dev/null 2>&1 && \
	find $(SRC_DIR) $(INC_DIR) -name '*.c' -o -name '*.h' | xargs clang-format -i || \
	echo "clang-format não disponível"

.PHONY: all clean valgrind check format