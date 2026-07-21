TARGET = calculator_app
TEST_TARGET = test_calculator_app

CC = gcc
CXX = g++

CFLAGS = -Wall -Wextra -Wpedantic -Werror -std=gnu17 -I. -MMD -MP
CXXFLAGS = -Wall -Wextra -Wpedantic -Werror -std=c++17 -I. -MMD -MP

BUILD_DIR = build
PLUGIN_DIR = plugins
BUILD_PLUGIN_DIR = $(BUILD_DIR)/plugins

PLUGIN_SRCS = $(wildcard $(PLUGIN_DIR)/*.c)
PLUGIN_SOS = $(patsubst $(PLUGIN_DIR)/%.c, $(BUILD_PLUGIN_DIR)/%.so, $(PLUGIN_SRCS))

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_PLUGIN_DIR)/%.so: $(PLUGIN_DIR)/%.c plugin_interface.h
	@mkdir -p $(BUILD_PLUGIN_DIR)
	$(CC) -shared -fPIC $(CFLAGS) $< -o $@
	@echo "Plugin shared library built: $@"

$(BUILD_DIR)/$(TARGET): main.c $(PLUGIN_SOS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) main.c -ldl -o $@
	@echo "Main app compiled: $@"

$(BUILD_DIR)/$(TEST_TARGET): test_calculator_plugins.cpp $(PLUGIN_SOS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) test_calculator_plugins.cpp -ldl -lgtest -lgtest_main -lpthread -o $@
	@echo "Test suite compiled: $@"

test: $(BUILD_DIR)/$(TEST_TARGET)
	@echo "========================================"
	@echo "Running unit tests for plugins..."
	@echo "========================================"
	./$(BUILD_DIR)/$(TEST_TARGET)

run: all
	@./$(BUILD_DIR)/$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
	@echo "Build directory removed."

re: clean all

.PHONY: all clean test run re