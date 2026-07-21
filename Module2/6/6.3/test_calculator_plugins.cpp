#include "plugin_interface.h"
#include <gtest/gtest.h>
#include <dlfcn.h>

TEST(PluginTest, LoadAddPlugin) {
    void *handle = dlopen("./build/plugins/add.so", RTLD_LAZY);
    ASSERT_NE(handle, nullptr);

    calc_plugin_t *info = (calc_plugin_t*)dlsym(handle, "plugin_info");
    ASSERT_NE(info, nullptr);

    EXPECT_STREQ(info->name, "Addition");
    EXPECT_STREQ(info->symbol, "+");

    double result = 0.0;
    EXPECT_EQ(info->op(5.0, 3.0, &result), CALC_OK);
    EXPECT_DOUBLE_EQ(result, 8.0);

    dlclose(handle);
}

TEST(PluginTest, LoadDivPlugin) {
    void *handle = dlopen("./build/plugins/div.so", RTLD_LAZY);
    ASSERT_NE(handle, nullptr);

    calc_plugin_t *info = (calc_plugin_t*)dlsym(handle, "plugin_info");
    ASSERT_NE(info, nullptr);

    double result = 0.0;
    EXPECT_EQ(info->op(10.0, 2.0, &result), CALC_OK);
    EXPECT_DOUBLE_EQ(result, 5.0);

    EXPECT_EQ(info->op(5.0, 0.0, &result), CALC_ERR_DIV_BY_ZERO);

    dlclose(handle);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}