#include <gtest/gtest.h>
#include <mqueue.h>
#include "chat_p2p.h"

TEST(ChatP2PTest, ParseArgs) {
    char name_buf[MAX_NAME_LEN];
    char arg0[] = "program";
    char arg1[] = "myqueue";
    char *argv[] = {arg0, arg1};

    EXPECT_EQ(parse_chat_args(2, argv, name_buf, sizeof(name_buf)), CHAT_SUCCESS);
    EXPECT_STREQ(name_buf, "/myqueue");
}

TEST(ChatP2PTest, ParseArgsWithSlash) {
    char name_buf[MAX_NAME_LEN];
    char arg0[] = "program";
    char arg1[] = "/myqueue";
    char *argv[] = {arg0, arg1};

    EXPECT_EQ(parse_chat_args(2, argv, name_buf, sizeof(name_buf)), CHAT_SUCCESS);
    EXPECT_STREQ(name_buf, "/myqueue");
}

TEST(ChatP2PTest, InitSessionCreatorAndJoiner) {
    const char *qname = "/test_queue_p2p";
    mq_unlink("/test_queue_p2p_1");
    mq_unlink("/test_queue_p2p_2");

    chat_session_t session1;
    ASSERT_EQ(init_chat_session(qname, &session1), CHAT_SUCCESS);
    EXPECT_EQ(session1.is_creator, 1);

    chat_session_t session2;
    ASSERT_EQ(init_chat_session(qname, &session2), CHAT_SUCCESS);
    EXPECT_EQ(session2.is_creator, 0);

    cleanup_chat_session(&session2);
    cleanup_chat_session(&session1);
}