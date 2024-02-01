#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> /*printk*/
#include <linux/platform_device.h>      /* For platform devices */
#include <linux/cdev.h>
#include <linux/fs.h>
#include "platform_data.h"

#include <linux/slab.h> /*kmalloc*/
#include <linux/mod_devicetable.h>
#include <linux/kdev_t.h>

static int my_pdrv_probe (struct platform_device *pdev);
static int my_pdrv_remove(struct platform_device *pdev);

/* Structure represents driver private data */
struct dummydrv_private_data {
    int total_device;
    dev_t device_number_base;
    struct class *class_dummy;
    struct device *device_dummy;
};
/* Structure represents device private data */
struct dummydev_private_data {
    struct dummydev_platform_data pdata;
    dev_t dev_num;
    char *buffer;
    struct cdev cdev;
};

static struct dummydrv_private_data m_dummydrv_data;
// static unsigned int major; /* major number for device */
// static struct class *dummy_class;
// static struct cdev dummy_cdev;

static struct platform_driver mypdrv = {
    .probe      = my_pdrv_probe,
    .remove     = my_pdrv_remove,
    .driver     = {
        .name     = "platform-dummy-char",
        .owner    = THIS_MODULE,
    },
};

int dummy_open(struct inode * inode, struct file * filp)
{
    pr_info("Someone tried to open me\n");
    return 0;
}

int dummy_release(struct inode * inode, struct file * filp)
{
    pr_info("Someone closed me\n");
    return 0;
}

ssize_t dummy_read (struct file *filp, char __user * buf, size_t count,
                                loff_t * offset)
{
    pr_info("Nothing to read guy\n");
    return 0;
}

ssize_t dummy_write(struct file * filp, const char __user * buf, size_t count,
                                loff_t * offset)
{
    pr_info("Can't accept any data guy\n");
    return count;
}

struct file_operations dummy_fops = {
    open:       dummy_open,
    release:    dummy_release,
    read:       dummy_read,
    write:      dummy_write,
};

static int my_pdrv_probe (struct platform_device *pdev)
{
    int ret = 0;
    struct dummydev_platform_data *p_data;
    struct dummydev_private_data *p_device_data;

    pr_info("Device was deteced\r\n");

    p_data = (struct dummydev_platform_data*)dev_get_platdata(&pdev->dev);
    if (p_data == NULL)
    {
        pr_err("No platform data available\r\n");
        return -EINVAL;
    }
    p_device_data = devm_kzalloc(&pdev->dev, sizeof(struct dummydev_private_data), GFP_KERNEL);
    if (p_device_data == NULL)
    {
        pr_err("Cannot allocate memory for device private data\r\n");
        return -ENOMEM;
    }
    /*Get platform data*/
    dev_set_drvdata(&pdev->dev, p_device_data);

    p_device_data->pdata = *p_data;
    pr_info("Device size :%d\r\n", p_device_data->pdata.size);
    pr_info("Device permisson: %d\r\n", p_device_data->pdata.permission);
    pr_info("Device serial number:%s\r\n", p_device_data->pdata.serial_number);

    p_device_data->buffer = devm_kzalloc(&pdev->dev, p_device_data->pdata.size, GFP_KERNEL);
    if (p_device_data->buffer == NULL)
    {
        pr_err("Cannot allocate memory for buffer data\r\n");
        return -ENOMEM;
    }
    p_device_data->dev_num = m_dummydrv_data.device_number_base + pdev->id;
    cdev_init(&p_device_data->cdev, &dummy_fops);
    ret = cdev_add(&p_device_data->cdev, p_device_data->dev_num, 1);
    if (ret < 0)
    {
        pr_err("cdev add failed\r\n");
        return ret;
    }
    m_dummydrv_data.device_dummy = device_create(m_dummydrv_data.class_dummy, &pdev->dev, p_device_data->dev_num, NULL, "platform_dummy_char");
    if (IS_ERR(m_dummydrv_data.device_dummy))
    {
        pr_err("Error createing sdma test class device\r\n");
        class_destroy(m_dummydrv_data.class_dummy);
        unregister_chrdev_region(m_dummydrv_data.device_number_base, 4);
        cdev_del(&p_device_data->cdev);
        return -1;
    }
    m_dummydrv_data.total_device++;
    pr_info("Platform dummy char device probed");
    return 0;
}

static int my_pdrv_remove(struct platform_device *pdev)
{
    struct dummydev_private_data *device_data = dev_get_drvdata(&pdev->dev);

    device_destroy(m_dummydrv_data.class_dummy, device_data->dev_num);
    cdev_del(&device_data->cdev);
    if (m_dummydrv_data.total_device)
    {
        m_dummydrv_data.total_device--;
    }
    
    pr_info("dummy char device was removed\n");
    return 0;
}

static int __init char_platform_driver_init(void)
{
    int error = 0;

    /* Get a range of minor numbers (starting with 0) to work with */
    error = alloc_chrdev_region(&m_dummydrv_data.device_number_base, 0, 4, "platform_dummy_char");
    if (error < 0) {
        pr_err("Can't get major number\n");
        return error;
    }
    m_dummydrv_data.class_dummy = class_create(THIS_MODULE, "platform_dummy_class");
    if (IS_ERR(m_dummydrv_data.class_dummy)) {
        pr_err("Error creating sdma test module class.\n");
        unregister_chrdev_region(m_dummydrv_data.device_number_base, 4);
        return PTR_ERR(m_dummydrv_data.class_dummy);
    }

    error = platform_driver_register(&mypdrv);
    if (error < 0)
    {
        class_destroy(m_dummydrv_data.class_dummy);
        unregister_chrdev_region(m_dummydrv_data.device_number_base, 4);
    }
    pr_info("Platform driver module loaded\n");
    return 0;
}

static void __exit char_platform_driver_exit(void) {
    platform_driver_unregister(&mypdrv);
    class_destroy(m_dummydrv_data.class_dummy);
    unregister_chrdev_region(m_dummydrv_data.device_number_base, 4);
    pr_info("Platform driver module unloaded\n");
}
module_init(char_platform_driver_init);
module_exit(char_platform_driver_exit);

MODULE_AUTHOR("Darwin_Tran<Darwin_Tran@vn.gemteks.com>");
MODULE_LICENSE("GPL");