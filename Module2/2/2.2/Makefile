TARGET = calculator_app

CC = gcc
STD = -std=gnu17

SRC_DIR = .
BUILD_DIR = build

CFLAGS = $(STD) -Wall -Wextra -Wpedantic -Werror -I$(SRC_DIR) -MMD -MP
LDFLAGS = 

ifeq ($(DEBUG), 1)
    CFLAGS += -g -O0 -fsanitize=address,undefined
    LDFLAGS += -fsanitize=address,undefined
    BUILD_TYPE = [DEBUG]
else
    CFLAGS += -O3
    BUILD_TYPE = [RELEASE]
endif

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

all: print_info $(BUILD_DIR)/$(TARGET)

print_info:
	@echo "Сборка проекта в режиме: $(BUILD_TYPE)"

$(BUILD_DIR)/$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Исполняемый файл собран: $@"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)
	@echo "Директория сборки '$(BUILD_DIR)' удалена."

run: all
	@echo "Запуск"
	@./$(BUILD_DIR)/$(TARGET) $(ARGS)

re: clean all

.PHONY: all print_info clean run re