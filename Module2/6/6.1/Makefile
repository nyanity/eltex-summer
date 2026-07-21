TARGET = contacts_dlol_app
LIB_TARGET = libcontacts.a
TEST_TARGET = test_contacts_dlol_app

CC = gcc
CXX = g++
AR = ar

CFLAGS = -Wall -Wextra -Wpedantic -Werror -std=c17 -I. -MMD -MP
CXXFLAGS = -Wall -Wextra -Wpedantic -Werror -std=c++17 -I. -MMD -MP
ARFLAGS = rcs

BUILD_DIR = build

SRC_LIB = contacts.c
OBJ_LIB = $(BUILD_DIR)/contacts.o

SRC_APP = main.c
OBJ_APP = $(BUILD_DIR)/main.o

SRC_TEST = test_contacts.cpp
OBJ_TEST = $(BUILD_DIR)/test_contacts.o

DEPS = $(OBJ_LIB:.o=.d) $(OBJ_APP:.o=.d) $(OBJ_TEST:.o=.d)

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(LIB_TARGET): $(OBJ_LIB)
	@mkdir -p $(BUILD_DIR)
	$(AR) $(ARFLAGS) $@ $(OBJ_LIB)
	@echo "Static library archived: $@"

$(BUILD_DIR)/$(TARGET): $(OBJ_APP) $(BUILD_DIR)/$(LIB_TARGET)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJ_APP) -L$(BUILD_DIR) -lcontacts -o $@
	@echo "Executable linked with static library: $@"

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TEST_TARGET): $(OBJ_TEST) $(BUILD_DIR)/$(LIB_TARGET)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(OBJ_TEST) -L$(BUILD_DIR) -lcontacts -lgtest -lgtest_main -lpthread -o $@
	@echo "Test executable linked with static library: $@"

test: $(BUILD_DIR)/$(TEST_TARGET)
	@echo "========================================"
	@echo "Running unit tests using static library..."
	@echo "========================================"
	./$(BUILD_DIR)/$(TEST_TARGET)

test_static: $(BUILD_DIR)/$(LIB_TARGET)
	file $(BUILD_DIR)/$(LIB_TARGET)
	$(AR) -t $(BUILD_DIR)/$(LIB_TARGET)
	nm $(BUILD_DIR)/$(LIB_TARGET)

run: all
	@./$(BUILD_DIR)/$(TARGET) $(ARGS)

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)
	@echo "Build directory removed."

re: clean all

.PHONY: all clean test run re