#include "contacts.h"
#include <gtest/gtest.h>
#include <algorithm>

int get_tree_height(contact_node_t *node) {
    if (!node) return 0;
    return 1 + std::max(get_tree_height(node->left), get_tree_height(node->right));
}

TEST(ContactsTreeTest, SortedInOrderTraversal) {
    contacts_t book;
    contacts_init(&book);

    record_t r1, r2, r3;
    record_init(&r1); record_set_first_name(&r1, "Zoe"); record_set_last_name(&r1, "Baker");
    record_init(&r2); record_set_first_name(&r2, "Alex"); record_set_last_name(&r2, "Adams");
    record_init(&r3); record_set_first_name(&r3, "John"); record_set_last_name(&r3, "Clark");

    EXPECT_EQ(contacts_add(&book, &r1), CONTACTS_OK);
    EXPECT_EQ(contacts_add(&book, &r2), CONTACTS_OK);
    EXPECT_EQ(contacts_add(&book, &r3), CONTACTS_OK);

    ASSERT_EQ(contacts_get_count(&book), 3);

    EXPECT_STREQ(contacts_get_at(&book, 0)->last_name.data, "Adams");
    EXPECT_STREQ(contacts_get_at(&book, 1)->last_name.data, "Baker");
    EXPECT_STREQ(contacts_get_at(&book, 2)->last_name.data, "Clark");

    record_free(&r1);
    record_free(&r2);
    record_free(&r3);
    contacts_free(&book);
}

TEST(ContactsTreeTest, PeriodicBalancingOptimizaton) {
    contacts_t book;
    contacts_init(&book);
    book.balance_threshold = 999;

    for (int i = 0; i < 5; i++) {
        record_t r;
        record_init(&r);
        char first[16], last[16];
        sprintf(first, "User%d", i);
        sprintf(last, "Surname%d", i);
        record_set_first_name(&r, first);
        record_set_last_name(&r, last);
        contacts_add(&book, &r);
        record_free(&r);
    }

    EXPECT_EQ(get_tree_height(book.root), 5);

    contacts_balance(&book);

    EXPECT_EQ(get_tree_height(book.root), 3);

    contacts_free(&book);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}