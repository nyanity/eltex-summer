#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define PROC_ENTRY_NAME "my_proc_file"
#define BUFFER_SIZE 512
#define PROC_PERMS 0666

MODULE_AUTHOR("Ivan Ivanov <ivan@example.com>");
MODULE_DESCRIPTION("Proc file exchange kernel module using proc_ops");
MODULE_LICENSE("GPL");

static char proc_buffer[BUFFER_SIZE];
static size_t proc_buffer_len = 0;

static ssize_t my_proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos)
{
    if (*pos >= proc_buffer_len) {
        return 0;
    }

    if (count > proc_buffer_len - *pos) {
        count = proc_buffer_len - *pos;
    }

    if (copy_to_user(usr_buf, proc_buffer + *pos, count)) {
        pr_err("Failed to copy data to user space\n");
        return -EFAULT;
    }

    *pos += count;
    return count;
}

static ssize_t my_proc_write(struct file *file, const char __user *usr_buf, size_t count, loff_t *pos)
{
    size_t to_copy = (count < BUFFER_SIZE - 1) ? count : BUFFER_SIZE - 1;

    if (copy_from_user(proc_buffer, usr_buf, to_copy)) {
        pr_err("Failed to copy data from user space\n");
        return -EFAULT;
    }

    proc_buffer[to_copy] = '\0';
    proc_buffer_len = to_copy;

    if (proc_buffer_len > 0 && proc_buffer[proc_buffer_len - 1] == '\n') {
        proc_buffer[proc_buffer_len - 1] = '\0';
        proc_buffer_len--;
    }

    pr_info("Received from userspace: %s\n", proc_buffer);
    return count;
}

static const struct proc_ops my_proc_ops = {
    .proc_read = my_proc_read,
    .proc_write = my_proc_write,
};

static int __init proc_init(void)
{
    struct proc_dir_entry *entry;

    entry = proc_create(PROC_ENTRY_NAME, PROC_PERMS, NULL, &my_proc_ops);
    if (!entry) {
        pr_err("Failed to create proc entry\n");
        return -ENOMEM;
    }

    pr_info("Proc entry /proc/%s created successfully\n", PROC_ENTRY_NAME);
    return 0;
}

static void __exit proc_exit(void)
{
    remove_proc_entry(PROC_ENTRY_NAME, NULL);
    pr_info("Proc entry /proc/%s removed\n", PROC_ENTRY_NAME);
}

module_init(proc_init);
module_exit(proc_exit);