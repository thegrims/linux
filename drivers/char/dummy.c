#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/types.h>
 
#include "dummy.h"
 
#define FIRST_MINOR 0
#define MINOR_CNT 1
 
static dev_t dev;
static struct cdev c_dev;
static struct class *cl;
 
static int my_open(struct inode *i, struct file *f)
{
    printk("CS444 Dummy driver open\r\n");
    return 0;
}
static int my_close(struct inode *i, struct file *f)
{
    printk("CS444 Dummy driver close\r\n");
    return 0;
}

static ssize_t dummy_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    printk("CS444 Dummy driver read\r\n");
    snprintf(buf, size, "Hey there, I'm a dummy!\r\n");
    return strlen(buf);
}

static ssize_t dummy_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
    char lcl_buf[64];

    memset(lcl_buf, 0, sizeof(lcl_buf));

    if (copy_from_user(lcl_buf, buf, min(size, sizeof(lcl_buf))))
        {
            return -EACCES;
        }

    printk("CS444 Dummy driver write %ld bytes: %s\r\n", size, lcl_buf);

    return size;
}
 
static struct file_operations dummy_fops =
{
    .owner = THIS_MODULE,
    .open = my_open,
    .read = dummy_read,
    .write = dummy_write,
    .release = my_close
};
 
static int __init dummy_init(void)
{
    int ret;
    struct device *dev_ret;
 
    // Allocate the device
    if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, "cs444_dummy")) < 0)
    {
        return ret;
    }
 
    cdev_init(&c_dev, &dummy_fops);
 
    if ((ret = cdev_add(&c_dev, dev, MINOR_CNT)) < 0)
    {
        return ret;
    }
     
    // Allocate the /dev device (/dev/cs444_dummy)
    if (IS_ERR(cl = class_create(THIS_MODULE, "char")))
    {
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, MINOR_CNT);
        return PTR_ERR(cl);
    }
    if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, "cs444_dummy")))
    {
        class_destroy(cl);
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, MINOR_CNT);
        return PTR_ERR(dev_ret);
    }

    printk("CS444 Dummy Driver has been loaded!\r\n");
 
    return 0;
}
 
static void __exit dummy_exit(void)
{
    device_destroy(cl, dev);
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, MINOR_CNT);
}
 
module_init(dummy_init);
module_exit(dummy_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kyson Montague <montaguk@oregonstate.edu>");
MODULE_DESCRIPTION("CS444 Demo Dummy Driver");