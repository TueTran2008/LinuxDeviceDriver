#include <linux/module.h>
#include <linux/platform_device.h>
#include "platform_data.h"

struct dummydev_platform_data dummy_dev_data[] = 
{
    [0] = {
        .size = 512,
        .permission = RDWR,
        .serial_number = "DUMMY_DEV1"
    },
    [1] = 
    {
        .size = 512,
        .permission = RDWR,
        .serial_number = "DUMMY_DEV2"
    },
    [2] =
    {
        .size = 1024,
        .permission = RDONLY,
        .serial_number = "DUMMY_DEV3"
    },
    [3] =
    {
        .size = 2048,
        .permission = WRONLY,
        .serial_number = "DUMMY_DEV4"
    },
};

/* Callback to free the device after all references have gone away */
void dummy_dev_release(struct device *dev) {
    // TODO: 
    pr_info("Device released\n");
}
/* Create two platform device */
struct platform_device platform_dummy_dev_1 = {
    .name = "dummydev-Ax",
    .id = 0,
    .dev = {
        .platform_data = &dummy_dev_data[0],
        .release = dummy_dev_release
    }
};

struct platform_device platform_dummy_dev_2 = {
    .name = "dummydev-Bx",
    .id = 1,
    .dev = {
        .platform_data = &dummy_dev_data[1],
        .release = dummy_dev_release
    }
};


struct platform_device platform_dummy_dev_3 = {
    .name = "dummydev-Cx",
    .id = 2,
    .dev = {
        .platform_data = &dummy_dev_data[2],
        .release = dummy_dev_release
    }
};

struct platform_device platform_dummy_dev_4 = {
    .name = "dummydev-Dx",
    .id = 3,
    .dev = {
        .platform_data = &dummy_dev_data[3],
        .release = dummy_dev_release
    }
};

struct platform_device *platform_dummydevs[] = {
    &platform_dummy_dev_1,
    &platform_dummy_dev_2,
    &platform_dummy_dev_3,
    &platform_dummy_dev_4
};

static int __init dummydev_platform_init(void) {
    /* Register platform-level device */
    platform_add_devices(platform_dummydevs, ARRAY_SIZE(platform_dummydevs));

    pr_info("Platform device setup module loaded\n");

    return 0;
}

static void __exit dummydev_platform_exit(void) {
    /* Unregister platform-level device */
    platform_device_unregister(&platform_dummy_dev_1);
    platform_device_unregister(&platform_dummy_dev_2);
    platform_device_unregister(&platform_dummy_dev_3);
    platform_device_unregister(&platform_dummy_dev_4);

    pr_info("Platform device setup module unloaded\n");
}

module_init(dummydev_platform_init);
module_exit(dummydev_platform_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Darwin Tran");
MODULE_DESCRIPTION("Register platform devices with the Linux kernel");
MODULE_INFO(board,"RPI 2");