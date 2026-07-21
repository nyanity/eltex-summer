TARGET = contacts_dlol_app
TEST_TARGET = test_contacts_dlol_app

CC = gcc
CXX = g++

STD = -std=gnu17
CXX_STD = -std=c++17

SRC_DIR = .
BUILD_DIR = build

CFLAGS = $(STD) -Wall -Wextra -Wpedantic -Werror -I$(SRC_DIR) -MMD -MP
CXXFLAGS = $(CXX_STD) -Wall -Wextra -Wpedantic -Werror -I$(SRC_DIR) -MMD -MP
LDFLAGS = 

ifeq ($(DEBUG), 1)
    CFLAGS += -g -O0 -fsanitize=address,undefined
    CXXFLAGS += -g -O0 -fsanitize=address,undefined
    LDFLAGS += -fsanitize=address,undefined
    BUILD_TYPE = [DEBUG]
else
    CFLAGS += -O3
    CXXFLAGS += -O3
    BUILD_TYPE = [RELEASE]
endif

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

LIB_OBJS = $(filter-out $(BUILD_DIR)/main.o, $(OBJS))

TEST_SRCS = $(wildcard $(SRC_DIR)/*.cpp)
TEST_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(TEST_SRCS))

DEPS = $(OBJS:.o=.d) $(TEST_OBJS:.o=.d)

all: print_info $(BUILD_DIR)/$(TARGET)

print_info:
	@echo "Сборка проекта в режиме: $(BUILD_TYPE)"

$(BUILD_DIR)/$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Исполняемый файл собран: $@"

$(BUILD_DIR)/$(TEST_TARGET): $(TEST_OBJS) $(LIB_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(TEST_OBJS) $(LIB_OBJS) -o $@ $(LDFLAGS) -lgtest -lgtest_main -lpthread
	@echo "Тестовый файл собран: $@"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)
	@echo "Директория сборки '$(BUILD_DIR)' удалена."

run: all
	@echo "Запуск"
	@./$(BUILD_DIR)/$(TARGET) $(ARGS)

test: $(BUILD_DIR)/$(TEST_TARGET)
	@echo "Запуск юнит-тестов"
	@./$(BUILD_DIR)/$(TEST_TARGET)

re: clean all

.PHONY: all print_info clean run re test