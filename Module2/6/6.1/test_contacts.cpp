#include "contacts.h"
#include <gtest/gtest.h>

TEST(ContactsListTest, SortedInsertion) {
    contacts_t book;
    contacts_init(&book);

    record_t r1, r2, r3;
    record_init(&r1); record_set_first_name(&r1, "Ivan"); record_set_last_name(&r1, "Sidorov");
    record_init(&r2); record_set_first_name(&r2, "Petr"); record_set_last_name(&r2, "Antonov");
    record_init(&r3); record_set_first_name(&r3, "Anna"); record_set_last_name(&r3, "Antonov");

    EXPECT_EQ(contacts_add(&book, &r1), CONTACTS_OK);
    EXPECT_EQ(contacts_add(&book, &r2), CONTACTS_OK);
    EXPECT_EQ(contacts_add(&book, &r3), CONTACTS_OK);

    ASSERT_EQ(contacts_get_count(&book), 3);

    EXPECT_STREQ(contacts_get_at(&book, 0)->last_name.data, "Antonov");
    EXPECT_STREQ(contacts_get_at(&book, 0)->first_name.data, "Anna");

    EXPECT_STREQ(contacts_get_at(&book, 1)->last_name.data, "Antonov");
    EXPECT_STREQ(contacts_get_at(&book, 1)->first_name.data, "Petr");

    EXPECT_STREQ(contacts_get_at(&book, 2)->last_name.data, "Sidorov");
    EXPECT_STREQ(contacts_get_at(&book, 2)->first_name.data, "Ivan");

    record_free(&r1);
    record_free(&r2);
    record_free(&r3);
    contacts_free(&book);
}

TEST(ContactsListTest, UpdateTriggersReorder) {
    contacts_t book;
    contacts_init(&book);

    record_t r1, r2;
    record_init(&r1); record_set_first_name(&r1, "Zoe"); record_set_last_name(&r1, "Baker");
    record_init(&r2); record_set_first_name(&r2, "Alex"); record_set_last_name(&r2, "Clark");

    contacts_add(&book, &r1);
    contacts_add(&book, &r2);

    EXPECT_STREQ(contacts_get_at(&book, 0)->last_name.data, "Baker");

    record_t updated;
    record_init(&updated);
    record_set_first_name(&updated, "Adam");
    record_set_last_name(&updated, "Adams"); // Adams идет до Baker

    EXPECT_EQ(contacts_update_at(&book, 1, &updated), CONTACTS_OK);

    EXPECT_STREQ(contacts_get_at(&book, 0)->last_name.data, "Adams");
    EXPECT_STREQ(contacts_get_at(&book, 1)->last_name.data, "Baker");

    record_free(&r1);
    record_free(&r2);
    record_free(&updated);
    contacts_free(&book);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}