#include <gtest/gtest.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include "raw_capture.h"

TEST(RawCaptureTest, ParseArgsChat) {
    char arg0[] = "program";
    char arg1[] = "-c";
    char arg2[] = "8888";
    char arg3[] = "5";
    char *argv[] = {arg0, arg1, arg2, arg3};

    capture_config_t config;
    EXPECT_EQ(parse_args(4, argv, &config), CAPTURE_SUCCESS);
    EXPECT_EQ(config.filter_type, FILTER_CHAT);
    EXPECT_EQ(config.port, 8888);
    EXPECT_EQ(config.max_packets, 5);
}

TEST(RawCaptureTest, ParseArgsDns) {
    char arg0[] = "program";
    char arg1[] = "--dns";
    char arg2[] = "53";
    char *argv[] = {arg0, arg1, arg2};

    capture_config_t config;
    EXPECT_EQ(parse_args(3, argv, &config), CAPTURE_SUCCESS);
    EXPECT_EQ(config.filter_type, FILTER_DNS);
    EXPECT_EQ(config.port, 53);
    EXPECT_EQ(config.max_packets, 10);
}

TEST(RawCaptureTest, StructHeadersSizeCheck) {
    EXPECT_EQ(sizeof(struct ethhdr), 14u);
}