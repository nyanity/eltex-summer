#include "ipv4_sim.h"
#include <gtest/gtest.h>

TEST(IPv4SimTest, ParseIPValid) {
    ipv4_addr_t ip = {0};
    EXPECT_EQ(parse_ip("192.168.1.1", &ip), IP_OK);
    EXPECT_EQ(ip.value, 0xC0A80101);

    EXPECT_EQ(parse_ip("255.255.255.255", &ip), IP_OK);
    EXPECT_EQ(ip.value, 0xFFFFFFFF);

    EXPECT_EQ(parse_ip("0.0.0.0", &ip), IP_OK);
    EXPECT_EQ(ip.value, 0x00000000);
}

TEST(IPv4SimTest, ParseIPInvalid) {
    ipv4_addr_t ip = {0};
    EXPECT_EQ(parse_ip("256.1.1.1", &ip), IP_ERR_INVALID_IP);
    EXPECT_EQ(parse_ip("1.1.1", &ip), IP_ERR_INVALID_IP);
    EXPECT_EQ(parse_ip("abc.def.ghi.jkl", &ip), IP_ERR_INVALID_IP);
}

TEST(IPv4SimTest, ParseMaskValid) {
    ipv4_addr_t mask = {0};
    EXPECT_EQ(parse_mask("255.255.255.0", &mask), IP_OK);
    EXPECT_EQ(mask.value, 0xFFFFFF00);

    EXPECT_EQ(parse_mask("255.255.0.0", &mask), IP_OK);
    EXPECT_EQ(mask.value, 0xFFFF0000);

    EXPECT_EQ(parse_mask("255.255.255.252", &mask), IP_OK);
    EXPECT_EQ(mask.value, 0xFFFFFFFC);
}

TEST(IPv4SimTest, ParseMaskInvalid) {
    ipv4_addr_t mask = {0};
    EXPECT_EQ(parse_mask("255.255.255.1", &mask), IP_ERR_INVALID_MASK);
    EXPECT_EQ(parse_mask("255.0.255.0", &mask), IP_ERR_INVALID_MASK);
    EXPECT_EQ(parse_mask("abc", &mask), IP_ERR_INVALID_MASK);
}

TEST(IPv4SimTest, IsInSubnet) {
    ipv4_addr_t gateway = {0xC0A80101};
    ipv4_addr_t mask = {0xFFFFFF00};   

    ipv4_addr_t local_ip = {0xC0A8012A}; 
    ipv4_addr_t remote_ip = {0xC0A8022A};

    EXPECT_TRUE(is_in_subnet(local_ip, gateway, mask));
    EXPECT_FALSE(is_in_subnet(remote_ip, gateway, mask));
}

TEST(IPv4SimTest, IPToString) {
    ipv4_addr_t ip = {0xC0A80101};
    char str[16];
    ip_to_string(ip, str);
    EXPECT_STREQ(str, "192.168.1.1");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}