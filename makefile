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
	
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	@echo "Limpeza concluída."

.PHONY: all clean