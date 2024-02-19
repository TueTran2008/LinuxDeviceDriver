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

static int my_pdrv_probe(struct platform_device *pdev);
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

/* Create dummy device configure */
enum pcdev_name {
    DUMMYDEV_A_CONF,
    DUMMYDEV_B_CONF,
    DUMMYDEV_C_CONF,
    DUMMYDEV_D_CONF,
};

struct device_configure {
    int configure_num1;
    int configure_num2;
};

struct device_configure pcdev_configure[] = {
    [DUMMYDEV_A_CONF] = {.configure_num1 = 40, .configure_num2 = 254 },
    [DUMMYDEV_B_CONF] = {.configure_num1 = 50, .configure_num2 = 253 },
    [DUMMYDEV_C_CONF] = {.configure_num1 = 60, .configure_num2 = 252 },
    [DUMMYDEV_D_CONF] = {.configure_num1 = 70, .configure_num2 = 251 },
};

struct platform_device_id pcdevs_ids[] = {
    [0] = {
        .name = "dummydev-Ax",
        .driver_data = DUMMYDEV_A_CONF
    },
    [1] = {
        .name = "dummydev-Bx",
        .driver_data = DUMMYDEV_B_CONF
    },
    [2] = {
        .name = "dummydev-Cx",
        .driver_data = DUMMYDEV_C_CONF
    },
    [3] = {
        .name = "dummydev-Dx",
        .driver_data = DUMMYDEV_D_CONF
    },
    {}
};

static struct platform_driver mypdrv = {
    .probe      = my_pdrv_probe,
    .remove     = my_pdrv_remove,
    .id_table = pcdevs_ids,
    .driver     = {
        .name     = "platform-dummy-char",
        .owner    = THIS_MODULE,
    },
};

static int check_permisstion(int permission, int access_mode)
{
    if (permission == RDWR)
    {
        return 0;
    }
    if ((permission == RDONLY) && (access_mode & FMODE_READ) && !(access_mode & FMODE_WRITE))
    {
        return 0;
    }
    if ((permission == WRONLY) && !(access_mode & FMODE_READ) && (access_mode & FMODE_WRITE))
    {
        return 0;
    }

    return -EPERM;
}
int dummy_open(struct inode * inode, struct file * filp)
{
    int minor_num = 0, ret = 0;
    struct dummydev_private_data *dummy_dev_data;

    /*Find out on which devices is being opened by the user space program*/
    minor_num = MINOR(inode->i_rdev);
    /*Get private data structure*/
    dummy_dev_data = container_of(inode->i_cdev, struct dummydev_private_data, cdev);

    /*Get device's private data*/
    filp->private_data = dummy_dev_data;

    ret = check_permisstion(dummy_dev_data->pdata.permission, filp->f_mode);
    if (!ret)
    {
        pr_info("Open was successfully\n");
    }
    else
    {
        pr_info("Open was unsuccessfully\n");
    }
    return 0;
}

int dummy_release(struct inode * inode, struct file * filp)
{
    pr_info("Released successfully\n");
    return 0;
}

ssize_t dummy_read(struct file *filp, char __user * buf, size_t count,
                                loff_t * offset)
{
    struct dummydev_private_data *dummy_dev_data = (struct dummydev_private_data *)filp->private_data;
    int max_size = 0;
    
    max_size = dummy_dev_data->pdata.size;

    pr_info("Write request %zu bytes \r\n", count);
    pr_info("Current file position = %lld\r\n", *offset);

    /* Ajust the count argument */
    if ((*offset + count) > max_size)
        count = max_size - *offset;

    if (copy_to_user(buf, dummy_dev_data->buffer, count))
        return -EFAULT;
    
    /* Update current file position */
    *offset += count;
    pr_info("Number of bytes successfully read = %zu\n", count);
    pr_info("Updated file position = %lld\n", *offset);
    return count;
}

ssize_t dummy_write(struct file * filp, const char __user * buf, size_t count,
                                loff_t * offset)
{
    struct dummydev_private_data *dummy_dev_data = (struct dummydev_private_data *)filp->private_data;
    int max_size = 0;
    
    max_size = dummy_dev_data->pdata.size;

    pr_info("Write request %zu bytes \r\n", count);
    pr_info("Current file position = %lld\r\n", *offset);

    /**/
   /* Ajust the count argument */
    if ((*offset + count) > max_size)
        count = max_size - *offset;

    if (!count)
        return -ENOMEM;

    if (copy_from_user(dummy_dev_data->buffer, buf, count))
        return -EFAULT;

    /* Update current file position */
    *offset += count;
    pr_info("Number of bytes successfully written = %zu\n", count);
    pr_info("Updated file position = %lld\n", *offset);

    return count;
}

struct file_operations dummy_fops = {
    .open =   dummy_open,
    .release = dummy_release,
    .read =    dummy_read,
    .write =     dummy_write,
    
};

static int my_pdrv_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct dummydev_platform_data *p_data;
    struct dummydev_private_data *p_device_data;

    pr_info("Device was deteced after fix\r\n");

    p_data = (struct dummydev_platform_data*)dev_get_platdata(&pdev->dev);

    pr_info("Probe got platform data\r\n");
    if (p_data == NULL)
    {
        pr_err("No platform data available\r\n");
        return -EINVAL;
    }
    p_device_data = devm_kzalloc(&pdev->dev, sizeof(*p_device_data), GFP_KERNEL);
    if (p_device_data == NULL)
    {
        pr_err("Cannot allocate memory for device private data\r\n");
        return -ENOMEM;
    }
    /*Get platform data*/
    dev_set_drvdata(&pdev->dev, p_device_data);

    p_device_data->pdata.size = p_data->size;
    p_device_data->pdata.permission = p_data->permission;
    p_device_data->pdata.serial_number = p_data->serial_number;
    
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
    p_device_data->cdev.owner = THIS_MODULE;
    ret = cdev_add(&p_device_data->cdev, p_device_data->dev_num, 1);
    if (ret < 0)
    {
        pr_err("cdev add failed\r\n");
        return ret;
    }
    // dummy_device = device_create(dummy_class,
    //                             NULL,   /* no parent device */
    //                             devt,   /* associated dev_t */
    //                             NULL,   /* no additional data */
    //                             "dummy_char"); /* device name */
    m_dummydrv_data.device_dummy = device_create(m_dummydrv_data.class_dummy, NULL, p_device_data->dev_num, NULL, "platform_dummy_char_%d", pdev->id);
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
    pr_info("--------------------\n");
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