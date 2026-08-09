#include <gtest/gtest.h>
#include "chat_protocol.h"

TEST(ChatProtocolTest, CheckHeaderSize) {
    EXPECT_EQ(sizeof(msg_header_t), 8u);
}

TEST(ChatProtocolTest, CheckPayloadSize) {
    EXPECT_EQ(sizeof(file_payload_t), 256u + 4u + MAX_FILE_DATA);
}