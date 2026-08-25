#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_AUTHOR("Mihailov Andrey <zwzardd@gmail.com>");
MODULE_DESCRIPTION("Hello World from kernel module.");
MODULE_LICENSE("Proprietary/MAL");

static int __init hello_init(void)
{
    pr_info("Hello, World!\n");
    return 0;
}

static void __exit hello_exit(void)
{
    pr_info("Goodbye, World!\n");
}

module_init(hello_init);
module_exit(hello_exit);