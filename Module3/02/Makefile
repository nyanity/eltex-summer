CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -Wpedantic -Werror -std=gnu17 -I. -MMD -MP
CXXFLAGS = -Wall -Wextra -Wpedantic -Werror -std=gnu++17 -I. -MMD -MP
LDFLAGS = 
TEST_LDFLAGS = -lgtest -lgtest_main -lpthread

BUILD_DIR = build
TARGET = $(BUILD_DIR)/pubsub
TEST_TARGET = $(BUILD_DIR)/test_runner

C_SRCS = pubsub.c main.c
C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRCS))

TEST_SRCS = test_pubsub.cpp
TEST_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(TEST_SRCS))

NON_MAIN_C_OBJS = $(filter-out $(BUILD_DIR)/main.o,$(C_OBJS))

DEPS = $(C_OBJS:.o=.d) $(TEST_OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(C_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(NON_MAIN_C_OBJS) $(TEST_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(TEST_LDFLAGS)

run: $(TARGET)
	@chmod +x test.sh
	./test.sh

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all test run clean

-include $(DEPS)