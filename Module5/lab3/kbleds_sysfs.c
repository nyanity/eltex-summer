#include <linux/init.h>
#include <linux/kd.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/vt.h>
#include <linux/vt_kern.h>
#include <linux/console_struct.h>
#include <linux/timer.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

#define BLINK_DELAY_MS 500
#define RESTORE_LEDS 0xFF
#define MAX_MASK_VALUE 7

MODULE_AUTHOR("Mihailov Andrey <zwzardd@gmail.com>");
MODULE_DESCRIPTION("Creating interface via sysfs for ioctl led beep.");
MODULE_LICENSE("Proprietary/MAL");

static struct timer_list my_timer;
static struct tty_driver *my_driver;
static struct kobject *kobj_ref;
static unsigned int led_mask = 0;
static bool blink_active = false;

static void my_timer_func(struct timer_list *t)
{
    static bool on = false;
    struct tty_struct *my_tty;

    if (!vc_cons[fg_console].d)
        return;

    my_tty = vc_cons[fg_console].d->port.tty;
    if (!my_tty || !my_driver || !my_driver->ops || !my_driver->ops->ioctl)
        return;

    on = !on;
    if (on) {
        my_driver->ops->ioctl(my_tty, KDSETLED, led_mask);
    } else {
        my_driver->ops->ioctl(my_tty, KDSETLED, 0);
    }

    if (blink_active) {
        mod_timer(&my_timer, jiffies + msecs_to_jiffies(BLINK_DELAY_MS));
    }
}

static ssize_t led_mask_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", led_mask);
}

static ssize_t led_mask_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    unsigned int val;
    int ret;
    struct tty_struct *my_tty;

    ret = kstrtouint(buf, 10, &val);
    if (ret < 0) {
        return ret;
    }

    if (val > MAX_MASK_VALUE) {
        pr_err("kbleds: Invalid mask value %u. Must be 0-%d.\n", val, MAX_MASK_VALUE);
        return -EINVAL;
    }

    led_mask = val;

    if (!vc_cons[fg_console].d)
        return -ENODEV;

    my_tty = vc_cons[fg_console].d->port.tty;
    if (!my_tty || !my_driver || !my_driver->ops || !my_driver->ops->ioctl) {
        pr_err("kbleds: TTY console or driver not ready\n");
        return -ENODEV;
    }

    if (led_mask == 0) {
        blink_active = false;
        del_timer_sync(&my_timer);
        my_driver->ops->ioctl(my_tty, KDSETLED, RESTORE_LEDS);
        pr_info("kbleds: Blinking stopped. LEDs restored to OS control.\n");
    } else {
        // Запуск или обновление маски мигания
        if (!blink_active) {
            blink_active = true;
            mod_timer(&my_timer, jiffies + msecs_to_jiffies(BLINK_DELAY_MS));
        }
        pr_info("kbleds: Blinking started with mask %u\n", led_mask);
    }

    return count;
}

static struct kobj_attribute led_mask_attribute = __ATTR(mask, 0664, led_mask_show, led_mask_store);

static int __init kbleds_init(void)
{
    int error = 0;
    int i;

    pr_info("kbleds: loading module\n");

    for (i = 0; i < MAX_NR_CONSOLES; i++) {
        if (!vc_cons[i].d)
            break;
        if (vc_cons[i].d->port.tty) {
            my_driver = vc_cons[i].d->port.tty->driver;
            break;
        }
    }

    if (!my_driver && vc_cons[fg_console].d && vc_cons[fg_console].d->port.tty) {
        my_driver = vc_cons[fg_console].d->port.tty->driver;
    }

    if (!my_driver) {
        pr_err("kbleds: tty driver not found. Run inside a physical VT console.\n");
        return -ENODEV;
    }

    timer_setup(&my_timer, my_timer_func, 0);

    kobj_ref = kobject_create_and_add("kbleds", kernel_kobj);
    if (!kobj_ref) {
        pr_err("kbleds: failed to create kobject\n");
        return -ENOMEM;
    }

    error = sysfs_create_file(kobj_ref, &led_mask_attribute.attr);
    if (error) {
        pr_err("kbleds: failed to create sysfs file\n");
        kobject_put(kobj_ref);
        return error;
    }

    pr_info("kbleds: sysfs interface initialized at /sys/kernel/kbleds/mask\n");
    return 0;
}

static void __exit kbleds_exit(void)
{
    pr_info("kbleds: unloading module\n");

    blink_active = false;
    del_timer_sync(&my_timer);

    if (vc_cons[fg_console].d && vc_cons[fg_console].d->port.tty && my_driver && my_driver->ops && my_driver->ops->ioctl) {
        my_driver->ops->ioctl(vc_cons[fg_console].d->port.tty, KDSETLED, RESTORE_LEDS);
    }

    if (kobj_ref) {
        sysfs_remove_file(kobj_ref, &led_mask_attribute.attr);
        kobject_put(kobj_ref);
    }
}

module_init(kbleds_init);
module_exit(kbleds_exit);