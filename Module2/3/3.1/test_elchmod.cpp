#include "elchmod.h"
#include <gtest/gtest.h>

TEST(ElchmodTest, ParseOctalValid) {
    mode_t mode = 0;
    EXPECT_EQ(parse_octal("755", &mode), ELCHMOD_OK);
    EXPECT_EQ(mode, 0755);

    EXPECT_EQ(parse_octal("000", &mode), ELCHMOD_OK);
    EXPECT_EQ(mode, 0000);
}

TEST(ElchmodTest, ParseOctalInvalid) {
    mode_t mode = 0;
    EXPECT_EQ(parse_octal("888", &mode), ELCHMOD_ERR_INVALID_FORMAT);
    EXPECT_EQ(parse_octal("abc", &mode), ELCHMOD_ERR_INVALID_FORMAT);
    EXPECT_EQ(parse_octal("7555", &mode), ELCHMOD_ERR_INVALID_FORMAT);
}

TEST(ElchmodTest, ParseSymbolicValid) {
    mode_t mode = 0;
    EXPECT_EQ(parse_symbolic("rwxr-xr-x", &mode), ELCHMOD_OK);
    EXPECT_EQ(mode, 0755);

    EXPECT_EQ(parse_symbolic("---------", &mode), ELCHMOD_OK);
    EXPECT_EQ(mode, 0000);
}

TEST(ElchmodTest, ParseSymbolicInvalid) {
    mode_t mode = 0;
    EXPECT_EQ(parse_symbolic("rwxr-xr-", &mode), ELCHMOD_ERR_INVALID_FORMAT);
    EXPECT_EQ(parse_symbolic("rwxr-xr-xx", &mode), ELCHMOD_ERR_INVALID_FORMAT);
    EXPECT_EQ(parse_symbolic("axr-xr-x", &mode), ELCHMOD_ERR_INVALID_FORMAT);
}

TEST(ElchmodTest, ApplyModificationOctal) {
    mode_t out = 0;
    EXPECT_EQ(apply_modification(0644, "755", &out), ELCHMOD_OK);
    EXPECT_EQ(out, 0755);
}

TEST(ElchmodTest, ApplyModificationAddSymbolic) {
    mode_t out = 0;
    EXPECT_EQ(apply_modification(0644, "u+x", &out), ELCHMOD_OK);
    EXPECT_EQ(out, 0744);

    EXPECT_EQ(apply_modification(0644, "a+x", &out), ELCHMOD_OK);
    EXPECT_EQ(out, 0755);
}

TEST(ElchmodTest, ApplyModificationRemoveSymbolic) {
    mode_t out = 0;
    EXPECT_EQ(apply_modification(0755, "g-w", &out), ELCHMOD_OK);
    EXPECT_EQ(out, 0755);

    EXPECT_EQ(apply_modification(0755, "o-x", &out), ELCHMOD_OK);
    EXPECT_EQ(out, 0754);
}

TEST(ElchmodTest, ModeConversions) {
    char sym[10];
    char oct[4];
    char bin[12];

    mode_to_symbolic(0755, sym);
    mode_to_octal(0755, oct);
    mode_to_binary(0755, bin);

    EXPECT_STREQ(sym, "rwxr-xr-x");
    EXPECT_STREQ(oct, "755");
    EXPECT_STREQ(bin, "111 101 101");
}

TEST(ElchmodTest, DatabaseOperations) {
    const char *test_file = "test_db_entry.txt";
    
    FILE *f = fopen(test_file, "w");
    ASSERT_NE(f, nullptr);
    fclose(f);

    mode_t initial_mode = 0600;
    EXPECT_EQ(db_set_mode(test_file, initial_mode), ELCHMOD_OK);

    mode_t retrieved_mode = 0;
    EXPECT_EQ(db_get_mode(test_file, &retrieved_mode), ELCHMOD_OK);
    EXPECT_EQ(retrieved_mode, initial_mode);

    remove(test_file);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}