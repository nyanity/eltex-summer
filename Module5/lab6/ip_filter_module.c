#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/inet.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#define PROC_ENTRY_NAME "ip_blacklist"
#define PROC_PERMS 0666

MODULE_AUTHOR("Mihailov Andrey <zwzardd@gmail.com>");
MODULE_DESCRIPTION("Netfilter.");
MODULE_LICENSE("GPL");

struct ip_node {
    __be32 ip_addr;
    struct list_head list;
};

static LIST_HEAD(blacklist_head);
static DEFINE_SPINLOCK(blacklist_lock);

static struct nf_hook_ops nf_ops;

static ssize_t my_proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos)
{
    char *temp_buf;
    size_t temp_buf_size = 1024;
    size_t len = 0;
    struct ip_node *entry;
    unsigned long flags;

    if (*pos > 0) return 0;

    temp_buf = kmalloc(temp_buf_size, GFP_KERNEL);
    if (!temp_buf) return -ENOMEM;

    spin_lock_irqsave(&blacklist_lock, flags);
    list_for_each_entry(entry, &blacklist_head, list) {
        if (len + 32 >= temp_buf_size) {
            break;
        }
        len += snprintf(temp_buf + len, temp_buf_size - len, "%pI4\n", &entry->ip_addr);
    }
    spin_unlock_irqrestore(&blacklist_lock, flags);

    if (copy_to_user(usr_buf, temp_buf, len)) {
        kfree(temp_buf);
        return -EFAULT;
    }

    *pos = len;
    kfree(temp_buf);
    return len;
}

static ssize_t my_proc_write(struct file *file, const char __user *usr_buf, size_t count, loff_t *pos)
{
    char buf[64];
    size_t len;
    char action;
    char *ip_str;
    __be32 ip;
    int ret;

    len = (count < sizeof(buf) - 1) ? count : sizeof(buf) - 1;
    if (copy_from_user(buf, usr_buf, len)) {
        return -EFAULT;
    }
    buf[len] = '\0';

    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[len - 1] = '\0';
        len--;
    }

    if (strcmp(buf, "clear") == 0) {
        struct ip_node *entry, *tmp;
        unsigned long flags;
        spin_lock_irqsave(&blacklist_lock, flags);
        list_for_each_entry_safe(entry, tmp, &blacklist_head, list) {
            list_del(&entry->list);
            kfree(entry);
        }
        spin_unlock_irqrestore(&blacklist_lock, flags);
        pr_info("ip_filter: Blacklist cleared\n");
        return count;
    }

    if (len < 2) {
        return -EINVAL;
    }

    action = buf[0];
    ip_str = buf + 1;

    ret = in4_pton(ip_str, strlen(ip_str), (u8 *)&ip, -1, NULL);
    if (ret == 0) {
        pr_err("ip_filter: Invalid IP address format: %s\n", ip_str);
        return -EINVAL;
    }

    if (action == '+') {
        struct ip_node *entry;
        unsigned long flags;
        bool found = false;

        spin_lock_irqsave(&blacklist_lock, flags);
        list_for_each_entry(entry, &blacklist_head, list) {
            if (entry->ip_addr == ip) {
                found = true;
                break;
            }
        }
        if (!found) {
            entry = kmalloc(sizeof(*entry), GFP_ATOMIC);
            if (!entry) {
                spin_unlock_irqrestore(&blacklist_lock, flags);
                return -ENOMEM;
            }
            entry->ip_addr = ip;
            list_add(&entry->list, &blacklist_head);
            pr_info("ip_filter: Added %pI4 to blacklist\n", &ip);
        }
        spin_unlock_irqrestore(&blacklist_lock, flags);
    } else if (action == '-') {
        struct ip_node *entry, *tmp;
        unsigned long flags;
        spin_lock_irqsave(&blacklist_lock, flags);
        list_for_each_entry_safe(entry, tmp, &blacklist_head, list) {
            if (entry->ip_addr == ip) {
                list_del(&entry->list);
                kfree(entry);
                pr_info("ip_filter: Removed %pI4 from blacklist\n", &ip);
            }
        }
        spin_unlock_irqrestore(&blacklist_lock, flags);
    } else {
        return -EINVAL;
    }

    return count;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops my_proc_ops = {
    .proc_read = my_proc_read,
    .proc_write = my_proc_write,
};
#else
static const struct file_operations my_proc_ops = {
    .owner = THIS_MODULE,
    .read = my_proc_read,
    .write = my_proc_write,
};
#endif

static unsigned int my_nf_hookfn(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
    struct iphdr *iph;
    struct ip_node *entry;
    unsigned long flags;
    bool block = false;

    if (!skb) return NF_ACCEPT;

    if (skb->protocol != htons(ETH_P_IP)) {
        return NF_ACCEPT;
    }

    iph = ip_hdr(skb);
    if (!iph) return NF_ACCEPT;

    spin_lock_irqsave(&blacklist_lock, flags);
    list_for_each_entry(entry, &blacklist_head, list) {
        if (iph->daddr == entry->ip_addr) {
            block = true;
            break;
        }
    }
    spin_unlock_irqrestore(&blacklist_lock, flags);

    if (block) {
        pr_info("ip_filter: BLOCKED outgoing packet to %pI4\n", &iph->daddr);
        return NF_DROP;
    }

    return NF_ACCEPT;
}

static int __init filter_init(void)
{
    struct proc_dir_entry *proc_entry;
    int ret;

    pr_info("ip_filter: Loading module\n");

    proc_entry = proc_create(PROC_ENTRY_NAME, PROC_PERMS, NULL, &my_proc_ops);
    if (!proc_entry) {
        pr_err("ip_filter: Failed to create proc entry\n");
        return -ENOMEM;
    }

    /* Set up Netfilter Hook */
    nf_ops.hook = my_nf_hookfn;
    nf_ops.pf = NFPROTO_IPV4;
    nf_ops.hooknum = NF_INET_LOCAL_OUT
    nf_ops.priority = NF_IP_PRI_FIRST;

    ret = nf_register_net_hook(&init_net, &nf_ops);
    if (ret < 0) {
        pr_err("ip_filter: Failed to register netfilter hook\n");
        remove_proc_entry(PROC_ENTRY_NAME, NULL);
        return ret;
    }

    pr_info("ip_filter: Module loaded. Manage via /proc/%s\n", PROC_ENTRY_NAME);
    return 0;
}

static void __exit filter_exit(void)
{
    struct ip_node *entry, *tmp;
    unsigned long flags;

    pr_info("ip_filter: Unloading module\n");

    nf_unregister_net_hook(&init_net, &nf_ops);

    /* Remove proc entry */
    remove_proc_entry(PROC_ENTRY_NAME, NULL);

    spin_lock_irqsave(&blacklist_lock, flags);
    list_for_each_entry_safe(entry, tmp, &blacklist_head, list) {
        list_del(&entry->list);
        kfree(entry);
    }
    spin_unlock_irqrestore(&blacklist_lock, flags);

    pr_info("ip_filter: Cleanup completed\n");
}

module_init(filter_init);
module_exit(filter_exit);