#include <gtest/gtest.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "posix_shm.h"

TEST(PosixShmTest, ParseArgsProducer) {
    char arg0[] = "program";
    char arg1[] = "-p";
    char *argv[] = {arg0, arg1};

    config_t config;
    EXPECT_EQ(parse_args(2, argv, &config), POSIX_SHM_SUCCESS);
    EXPECT_EQ(config.role, ROLE_PRODUCER);
}

TEST(PosixShmTest, ParseArgsConsumer) {
    char arg0[] = "program";
    char arg1[] = "-c";
    char *argv[] = {arg0, arg1};

    config_t config;
    EXPECT_EQ(parse_args(2, argv, &config), POSIX_SHM_SUCCESS);
    EXPECT_EQ(config.role, ROLE_CONSUMER);
}

TEST(PosixShmTest, AlignmentHelper) {
    EXPECT_EQ(ALIGN8(1), 8u);
    EXPECT_EQ(ALIGN8(8), 8u);
    EXPECT_EQ(ALIGN8(9), 16u);
}