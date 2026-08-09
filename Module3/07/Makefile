CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -Wpedantic -Werror -std=gnu17 -I. -MMD -MP
CXXFLAGS = -Wall -Wextra -Wpedantic -Werror -std=gnu++17 -I. -MMD -MP
LDFLAGS = -lpthread
TEST_LDFLAGS = -lgtest -lgtest_main -lpthread

BUILD_DIR = build
SERVER_TARGET = $(BUILD_DIR)/chat_server
CLIENT_TARGET = $(BUILD_DIR)/chat_client
TEST_TARGET = $(BUILD_DIR)/test_runner

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): $(BUILD_DIR)/chat_server.o
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(CLIENT_TARGET): $(BUILD_DIR)/chat_client.o
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

$(TEST_TARGET): $(BUILD_DIR)/test_chat.o
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(TEST_LDFLAGS)

run: all
	@chmod +x test.sh
	./test.sh

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all test run clean

-include $(BUILD_DIR)/*.d