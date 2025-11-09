CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./include
SRCDIR = src
SOURCES = $(SRCDIR)/main.c $(SRCDIR)/cpu_monitor.c $(SRCDIR)/memory_monitor.c $(SRCDIR)/io_monitor.c $(SRCDIR)/net_monitor.c
TARGET = resource-monitor
OBJS = $(SOURCES:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

test: $(TARGET)
	@echo "=== TESTANDO COM PROCESSO ATUAL ==="
	./$(TARGET) --pid $$ --detalhes

test-multiple: $(TARGET)
	@echo "=== TESTANDO MULTIPLOS PROCESSOS ==="
	./$(TARGET) --pids 1,$$ --intervalo 2

.PHONY: clean install test test-multiple