#include <gtest/gtest.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "pubsub.h"

TEST(PubSubTest, ParseArgsBroker) {
    char arg0[] = "program";
    char arg1[] = "-b";
    char *argv[] = {arg0, arg1};

    pubsub_config_t config;
    EXPECT_EQ(parse_args(2, argv, &config), PUBSUB_SUCCESS);
    EXPECT_EQ(config.role, ROLE_BROKER);
}

TEST(PubSubTest, ParseArgsPublisher) {
    char arg0[] = "program";
    char arg1[] = "-p";
    char arg2[] = "news";
    char *argv[] = {arg0, arg1, arg2};

    pubsub_config_t config;
    EXPECT_EQ(parse_args(3, argv, &config), PUBSUB_SUCCESS);
    EXPECT_EQ(config.role, ROLE_PUBLISHER);
    EXPECT_EQ(config.topic_count, 1u);
    EXPECT_STREQ(config.topics[0], "news");
}

TEST(PubSubTest, ParseArgsSubscriber) {
    char arg0[] = "program";
    char arg1[] = "-s";
    char arg2[] = "news";
    char arg3[] = "sports";
    char *argv[] = {arg0, arg1, arg2, arg3};

    pubsub_config_t config;
    EXPECT_EQ(parse_args(4, argv, &config), PUBSUB_SUCCESS);
    EXPECT_EQ(config.role, ROLE_SUBSCRIBER);
    EXPECT_EQ(config.topic_count, 2u);
    EXPECT_STREQ(config.topics[0], "news");
    EXPECT_STREQ(config.topics[1], "sports");
}

TEST(PubSubTest, ParseArgsInvalid) {
    char arg0[] = "program";
    char arg1[] = "-invalid";
    char *argv[] = {arg0, arg1};

    pubsub_config_t config;
    EXPECT_EQ(parse_args(2, argv, &config), PUBSUB_ERR_INVALID_ARGS);
}

TEST(PubSubTest, QueueCreation) {
    key_t key = get_default_key();
    int msqid = msgget(key, 0666);
    if (msqid >= 0) {
        msgctl(msqid, IPC_RMID, NULL);
    }

    msqid = msgget(key, IPC_CREAT | IPC_EXCL | 0666);
    ASSERT_GE(msqid, 0);

    msgctl(msqid, IPC_RMID, NULL);
}