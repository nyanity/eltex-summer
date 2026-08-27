#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define DEVICE_NAME "my_chardev"
#define CLASS_NAME "my_chardev_class"
#define BUFFER_SIZE 512

MODULE_AUTHOR("Mihailov Andrey <zwzardd@gmail.com>");
MODULE_DESCRIPTION("Chardev.");
MODULE_LICENSE("GPL");

static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class = NULL;
static struct device *my_device = NULL;

static char device_buffer[BUFFER_SIZE];
static size_t device_buffer_len = 0;

static int device_open(struct inode *inode, struct file *file)
{
    pr_info("chardev: Device opened\n");
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    pr_info("chardev: Device closed\n");
    return 0;
}

static ssize_t device_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos)
{
    if (*pos >= device_buffer_len) {
        return 0;
    }

    if (count > device_buffer_len - *pos) {
        count = device_buffer_len - *pos;
    }

    if (copy_to_user(usr_buf, device_buffer + *pos, count)) {
        pr_err("chardev: Failed to copy data to userspace\n");
        return -EFAULT;
    }

    *pos += count;
    pr_info("chardev: Read %zu bytes from device\n", count);
    return count;
}

static ssize_t device_write(struct file *file, const char __user *usr_buf, size_t count, loff_t *pos)
{
    size_t to_copy = (count < BUFFER_SIZE - 1) ? count : BUFFER_SIZE - 1;

    if (copy_from_user(device_buffer, usr_buf, to_copy)) {
        pr_err("chardev: Failed to copy data from userspace\n");
        return -EFAULT;
    }

    device_buffer[to_copy] = '\0';
    device_buffer_len = to_copy;

    if (device_buffer_len > 0 && device_buffer[device_buffer_len - 1] == '\n') {
        device_buffer[device_buffer_len - 1] = '\0';
        device_buffer_len--;
    }

    pr_info("chardev: Written %zu bytes: %s\n", count, device_buffer);
    return count;
}

static const struct file_operations chardev_fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
};

static int __init chardev_init(void)
{
    int ret;

    pr_info("chardev: Initializing character device\n");

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("chardev: Failed to allocate major number\n");
        return ret;
    }

    cdev_init(&my_cdev, &chardev_fops);
    my_cdev.owner = THIS_MODULE;
    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("chardev: Failed to add cdev\n");
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    my_class = class_create(CLASS_NAME);
#else
    my_class = class_create(THIS_MODULE, CLASS_NAME);
#endif
    if (IS_ERR(my_class)) {
        pr_err("chardev: Failed to create class\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(my_class);
    }

    my_device = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(my_device)) {
        pr_err("chardev: Failed to create device\n");
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(my_device);
    }

    pr_info("chardev: Device /dev/%s created successfully (Major: %u, Minor: %u)\n",
            DEVICE_NAME, MAJOR(dev_num), MINOR(dev_num));
    return 0;
}

static void __exit chardev_exit(void)
{
    pr_info("chardev: Unregistering character device\n");

    if (my_class) {
        device_destroy(my_class, dev_num);
        class_destroy(my_class);
    }
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);

    pr_info("chardev: Cleanup finished\n");
}

module_init(chardev_init);
module_exit(chardev_exit);