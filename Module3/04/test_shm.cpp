#include <gtest/gtest.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "shm_prod_cons.h"

TEST(ShmTest, ParseArgsProducer) {
    char arg0[] = "program";
    char arg1[] = "-p";
    char *argv[] = {arg0, arg1};

    config_t config;
    EXPECT_EQ(parse_args(2, argv, &config), SHM_SUCCESS);
    EXPECT_EQ(config.role, ROLE_PRODUCER);
}

TEST(ShmTest, ParseArgsConsumer) {
    char arg0[] = "program";
    char arg1[] = "-c";
    char *argv[] = {arg0, arg1};

    config_t config;
    EXPECT_EQ(parse_args(2, argv, &config), SHM_SUCCESS);
    EXPECT_EQ(config.role, ROLE_CONSUMER);
}

TEST(ShmTest, AlignmentHelper) {
    EXPECT_EQ(ALIGN8(1), 8u);
    EXPECT_EQ(ALIGN8(8), 8u);
    EXPECT_EQ(ALIGN8(9), 16u);
}

TEST(ShmTest, KeyGeneration) {
    key_t k = get_shm_key();
    EXPECT_NE(k, (key_t)-1);
}