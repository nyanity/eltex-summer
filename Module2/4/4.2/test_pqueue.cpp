#include "pqueue.h"
#include <gtest/gtest.h>

TEST(PQueueTest, BasicOperations) {
    pqueue_t q;
    pqueue_init(&q);

    EXPECT_EQ(pqueue_push(&q, "Low", 10), PQUEUE_OK);
    EXPECT_EQ(pqueue_push(&q, "High", 200), PQUEUE_OK);
    EXPECT_EQ(pqueue_push(&q, "Critical", 255), PQUEUE_OK);

    char *text = nullptr;
    uint8_t priority = 0;

    EXPECT_EQ(pqueue_pop_highest(&q, &text, &priority), PQUEUE_OK);
    EXPECT_STREQ(text, "Critical");
    EXPECT_EQ(priority, 255);
    free(text);

    pqueue_free(&q);
}

TEST(PQueueTest, PopAtLeast) {
    pqueue_t q;
    pqueue_init(&q);

    pqueue_push(&q, "Level 50", 50);
    pqueue_push(&q, "Level 100", 100);

    char *text = nullptr;
    uint8_t priority = 0;

    EXPECT_EQ(pqueue_pop_at_least(&q, 80, &text, &priority), PQUEUE_OK);
    EXPECT_STREQ(text, "Level 100");
    EXPECT_EQ(priority, 100);
    free(text);

    EXPECT_EQ(pqueue_pop_at_least(&q, 80, &text, &priority), PQUEUE_ERR_EMPTY);

    pqueue_free(&q);
}

TEST(PQueueTest, FIFOForSamePriority) {
    pqueue_t q;
    pqueue_init(&q);

    pqueue_push(&q, "First 10", 10);
    pqueue_push(&q, "Second 10", 10);

    char *text = nullptr;
    EXPECT_EQ(pqueue_pop_exact(&q, 10, &text), PQUEUE_OK);
    EXPECT_STREQ(text, "First 10");
    free(text);

    EXPECT_EQ(pqueue_pop_exact(&q, 10, &text), PQUEUE_OK);
    EXPECT_STREQ(text, "Second 10");
    free(text);

    pqueue_free(&q);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}