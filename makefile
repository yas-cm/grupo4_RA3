TARGET = resource_monitor

CC = gcc

SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include
TEST_DIR = tests

CFLAGS = -D_POSIX_C_SOURCE=200809L -Wall -Wextra -std=c99 -g -I$(INC_DIR)

LDFLAGS = -lrt -lm

SRCS = $(wildcard $(SRC_DIR)/*.c)

OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Testes
TEST_SRCS = $(wildcard $(TEST_DIR)/test_*.c)
TEST_BINS = $(patsubst $(TEST_DIR)/%.c, $(TEST_DIR)/%, $(TEST_SRCS))

# Objetos compartilhados (excluindo main.o para os testes)
SHARED_OBJS = $(filter-out $(OBJ_DIR)/main.o, $(OBJS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "✓ Compilação concluída com sucesso!"
	
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compilar testes
tests: $(TEST_BINS)

$(TEST_DIR)/test_%: $(TEST_DIR)/test_%.c $(SHARED_OBJS)
	$(CC) $(CFLAGS) $< $(SHARED_OBJS) -o $@ $(LDFLAGS)
	@echo "✓ Teste $@ compilado com sucesso!"

clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(TEST_BINS)
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

.PHONY: all clean valgrind check format tests