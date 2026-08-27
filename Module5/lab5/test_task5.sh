#!/bin/bash
# Test script for Task 5 (Netlink Module)

MODULE="netlink_module"
KO_FILE="${MODULE}.ko"
USER_SRC="user_app.c"
USER_BIN="./user_app"

echo "=== Testing Task 5 (Netlink Socket) ==="

if [ ! -f "$KO_FILE" ]; then
    echo "Error: $KO_FILE not found. Please run 'make' first."
    exit 1
fi

echo "1. Generating userspace test application..."
cat << 'EOF' > "$USER_SRC"
#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define NETLINK_USER 31
#define MAX_PAYLOAD 1024

struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;

int main() {
    sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (sock_fd < 0) {
        perror("socket");
        return -1;
    }

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); /* self pid */

    if (bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0) {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; /* Target: Linux Kernel */
    dest_addr.nl_groups = 0; /* Unicast */

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    if (!nlh) {
        perror("malloc");
        close(sock_fd);
        return -1;
    }
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;

    strcpy(NLMSG_DATA(nlh), "Hello from userspace!");

    memset(&iov, 0, sizeof(iov));
    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (struct sockaddr *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    printf("Sending message to kernel...\n");
    if (sendmsg(sock_fd, &msg, 0) < 0) {
        perror("sendmsg");
        close(sock_fd);
        free(nlh);
        return -1;
    }

    /* Receive message from kernel */
    printf("Waiting for message from kernel...\n");
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    if (recvmsg(sock_fd, &msg, 0) < 0) {
        perror("recvmsg");
        close(sock_fd);
        free(nlh);
        return -1;
    }

    printf("Received payload from kernel: \"%s\"\n", (char *)NLMSG_DATA(nlh));

    close(sock_fd);
    free(nlh);
    return 0;
}
EOF

echo "2. Compiling userspace test application..."
gcc "$USER_SRC" -o "$USER_BIN"
echo "   [OK] Compiled successfully."

echo "3. Inserting kernel module..."
sudo insmod "$KO_FILE"

echo "4. Running communication test..."
TEST_OUTPUT=$("$USER_BIN")
echo "$TEST_OUTPUT"

if echo "$TEST_OUTPUT" | grep -q "Hello from kernel!"; then
    echo "   [OK] Netlink loopback communication verified."
else
    echo "   [FAIL] Expected reply from kernel was not received."
    sudo rmmod "$MODULE"
    rm -f "$USER_SRC" "$USER_BIN"
    exit 1
fi

echo "5. Checking kernel logs (dmesg) for incoming netlink message..."
if dmesg | tail -n 20 | grep -q "netlink:"; then
    echo "   [OK] Found netlink entries in dmesg:"
    dmesg | tail -n 20 | grep "netlink:"
else
    echo "   [WARNING] No logs found in dmesg."
fi

echo "6. Removing kernel module..."
sudo rmmod "$MODULE"

echo "7. Cleaning up temporary application files..."
rm -f "$USER_SRC" "$USER_BIN"
echo "   [OK] Temp files removed."

echo "=== Task 5 Test Completed Successfully ==="