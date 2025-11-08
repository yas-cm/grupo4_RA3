CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
TARGET = monitor

SOURCES = src/main.c src/cpu_monitor.c

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET) src/*.o

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS += -g
debug: $(TARGET)

help:
	@echo "Comandos:"
	@echo "  make      - Compila"
	@echo "  make run  - Compila e executa"
	@echo "  make clean - Limpa"
	@echo "  make debug - Compila com debug"

.PHONY: clean run debug help