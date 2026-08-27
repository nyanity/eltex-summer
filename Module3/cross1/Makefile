CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Werror -std=gnu17 -I. -MMD -MP
LDFLAGS = 

BUILD_DIR = build
SERVER_TARGET = $(BUILD_DIR)/chat_server
CLIENT_TARGET = $(BUILD_DIR)/chat_client

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): $(BUILD_DIR)/server.o
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(CLIENT_TARGET): $(BUILD_DIR)/client.o
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean

-include $(BUILD_DIR)/*.d