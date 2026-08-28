#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

#define NETLINK_USER 31

MODULE_AUTHOR("Mihailov Andrey <zwzardd@gmail.com>");
MODULE_DESCRIPTION("Net.");
MODULE_LICENSE("GPL");

static struct sock *nl_sk = NULL;

static void nl_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    int pid;
    struct sk_buff *skb_out;
    int msg_size;
    const char *msg = "Hello from kernel!";
    int res;

    nlh = (struct nlmsghdr *)skb->data;
    pr_info("netlink: Received payload: %s\n", (char *)nlmsg_data(nlh));

    pid = nlh->nlmsg_pid; /* PID of sending process */
    msg_size = strlen(msg);

    skb_out = nlmsg_new(msg_size, GFP_KERNEL);
    if (!skb_out) {
        pr_err("netlink: Failed to allocate new skb\n");
        return;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0;
    strncpy(nlmsg_data(nlh), msg, msg_size);

    res = nlmsg_unicast(nl_sk, skb_out, pid);
    if (res < 0) {
        pr_err("netlink: Error while sending back to user\n");
    }
}

static int __init nl_init(void)
{
    struct netlink_kernel_cfg cfg = {
        .input = nl_recv_msg,
    };

    pr_info("netlink: Initializing netlink socket\n");
    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if (!nl_sk) {
        pr_err("netlink: Error creating socket.\n");
        return -ENOMEM;
    }

    pr_info("netlink: Socket created successfully\n");
    return 0;
}

static void __exit nl_exit(void)
{
    pr_info("netlink: Releasing netlink socket\n");
    if (nl_sk) {
        netlink_kernel_release(nl_sk);
    }
    pr_info("netlink: Socket released\n");
}

module_init(nl_init);
module_exit(nl_exit);