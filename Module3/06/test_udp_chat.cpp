#include <gtest/gtest.h>
#include "udp_chat.h"

TEST(UdpChatTest, ParseArgsDefaults) {
    char arg0[] = "program";
    char *argv[] = {arg0};

    udp_chat_config_t config;
    EXPECT_EQ(parse_args(1, argv, &config), UDP_CHAT_SUCCESS);
    EXPECT_EQ(config.port, DEFAULT_PORT);
    EXPECT_NE(config.nickname[0], '\0');
}

TEST(UdpChatTest, ParseArgsCustom) {
    char arg0[] = "program";
    char arg1[] = "Alice";
    char arg2[] = "9999";
    char *argv[] = {arg0, arg1, arg2};

    udp_chat_config_t config;
    EXPECT_EQ(parse_args(3, argv, &config), UDP_CHAT_SUCCESS);
    EXPECT_STREQ(config.nickname, "Alice");
    EXPECT_EQ(config.port, 9999);
}

TEST(UdpChatTest, ParseArgsInvalidPort) {
    char arg0[] = "program";
    char arg1[] = "Bob";
    char arg2[] = "invalid_port";
    char *argv[] = {arg0, arg1, arg2};

    udp_chat_config_t config;
    EXPECT_EQ(parse_args(3, argv, &config), UDP_CHAT_ERR_INVALID_ARGS);
}