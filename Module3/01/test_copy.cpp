#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include "copy_app.h"

TEST(CopyAppTest, ParseArgsValidWithoutFifo) {
    char arg0[] = "program";
    char arg1[] = "file1.txt";
    char arg2[] = "file2.txt";
    char *argv[] = {arg0, arg1, arg2};

    app_config_t config;
    copy_status_t status = parse_args(3, argv, &config);

    EXPECT_EQ(status, COPY_SUCCESS);
    EXPECT_EQ(config.fifo_path, nullptr);
    EXPECT_EQ(config.file_count, 2u);
    EXPECT_STREQ(config.filenames[0], "file1.txt");
    EXPECT_STREQ(config.filenames[1], "file2.txt");
}

TEST(CopyAppTest, ParseArgsValidWithFifo) {
    char arg0[] = "program";
    char arg1[] = "-p";
    char arg2[] = "myfifo";
    char arg3[] = "file1.txt";
    char *argv[] = {arg0, arg1, arg2, arg3};

    app_config_t config;
    copy_status_t status = parse_args(4, argv, &config);

    EXPECT_EQ(status, COPY_SUCCESS);
    ASSERT_NE(config.fifo_path, nullptr);
    EXPECT_STREQ(config.fifo_path, "myfifo");
    EXPECT_EQ(config.file_count, 1u);
    EXPECT_STREQ(config.filenames[0], "file1.txt");
}

TEST(CopyAppTest, ParseArgsInvalid) {
    char arg0[] = "program";
    char arg1[] = "-p";
    char *argv1[] = {arg0, arg1};

    app_config_t config;
    EXPECT_NE(parse_args(1, argv1, &config), COPY_SUCCESS);
    EXPECT_NE(parse_args(2, argv1, &config), COPY_SUCCESS);
}

TEST(CopyAppTest, CopyFilesUnnamedPipe) {
    const char *test_filename = "test_src_unnamed.txt";
    const char *copy_filename = "test_src_unnamed.txt.copy";
    std::string test_content = "Hello, OS Module 3 Task 1!";

    {
        std::ofstream ofs(test_filename);
        ofs << test_content;
    }

    char arg0[] = "program";
    char arg1[] = "test_src_unnamed.txt";
    char *argv[] = {arg0, arg1};

    app_config_t config;
    ASSERT_EQ(parse_args(2, argv, &config), COPY_SUCCESS);
    ASSERT_EQ(run_copy_process(&config), COPY_SUCCESS);

    std::ifstream ifs(copy_filename);
    std::string copied_content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    EXPECT_EQ(test_content, copied_content);

    std::remove(test_filename);
    std::remove(copy_filename);
}

TEST(CopyAppTest, CopyFilesNamedPipe) {
    const char *test_filename = "test_src_named.txt";
    const char *copy_filename = "test_src_named.txt.copy";
    const char *fifo_name = "test_fifo_tmp";
    std::string test_content = "Testing FIFO copying mechanism!";

    {
        std::ofstream ofs(test_filename);
        ofs << test_content;
    }

    char arg0[] = "program";
    char arg1[] = "-p";
    char arg2[] = "test_fifo_tmp";
    char arg3[] = "test_src_named.txt";
    char *argv[] = {arg0, arg1, arg2, arg3};

    app_config_t config;
    ASSERT_EQ(parse_args(4, argv, &config), COPY_SUCCESS);
    ASSERT_EQ(run_copy_process(&config), COPY_SUCCESS);

    std::ifstream ifs(copy_filename);
    std::string copied_content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    EXPECT_EQ(test_content, copied_content);

    std::remove(test_filename);
    std::remove(copy_filename);
    std::remove(fifo_name);
}