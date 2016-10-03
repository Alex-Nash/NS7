#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <asm/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DEBUG 1
#include "debug.h"
#include "axi_bram.h"

#define AXI_BRAM_BASE 	0x80000000
#define AXI_BRAM_WIDTH	0x00010000


u32 *axibram_pointer;

#define DEVICE_NAME "axi_bram"
#define CLASS_NAME "bram"

#define SUCCESS 0

static int majorNumber;
static int device_open = 0;
static struct class*  charClass  = NULL;
static struct device* charDevice = NULL;


static int bram_close(struct inode *inode, struct file *file)
{
    device_open--;

    DEBUG_PRINT("axi bram: close %s module", DEVICE_NAME);

    module_put(THIS_MODULE);

    return SUCCESS;
}

static long bram_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct bram_rw_data data;

    if (copy_from_user(&data, (struct bram_rw_data*)arg, sizeof(struct bram_rw_data)))
    {
        DEBUG_PRINT("axi bram: ioctl fail get arg");
        return -EACCES;
    }

    if(data.size > AXI_BRAM_WIDTH - data.offset)
    {
        DEBUG_PRINT("axi bram: ioctl fail, size more then memory width");
        return -EACCES;
    }

    u32 *value;

    value = kmalloc(data.size * sizeof(u32), GFP_KERNEL);

    switch (cmd)
    {
    case AXI_BRAM_WRITE:
        DEBUG_PRINT("axi bram: ioctl write value");
        if (copy_from_user(value, (u32 *)data.data, data.size * sizeof(u32)))
        {
            DEBUG_PRINT("axi bram: ioctl fail copy from user");
            kfree(value);
            return -EACCES;
        }
        memcpy(axibram_pointer + data.offset, value, data.size * sizeof(u32));
        DEBUG_PRINT("axi bram: ioctl write");
        break;
    case AXI_BRAM_READ:
        DEBUG_PRINT("axi bram: ioctl read value");
        memcpy(value, axibram_pointer + data.offset, data.size * sizeof(u32));

        if(copy_to_user(data.data, value, data.size * sizeof(u32)))
        {
            DEBUG_PRINT("axi bram: ioctl fail copy to user");
            kfree(value);
            return -EACCES;
        }
        break;
    default:
        kfree(value);
        return -ENOTTY;
    }

    kfree(value);

    return 0;
}

struct file_operations fops = {
    .open = bram_open,
    .release = bram_close,
    .unlocked_ioctl = bram_ioctl
};


static int __init mod_init(void)
{
    DEBUG_PRINT("axi bram: init %s module", DEVICE_NAME);
    axibram_pointer = ioremap_nocache(AXI_BRAM_BASE, AXI_BRAM_WIDTH);

    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0)
    {
        printk(KERN_ALERT "Axi-bram : failed to register a major number\n");
        return majorNumber;
    }
    DEBUG_PRINT("axi bram: register with major number %d", majorNumber);


    DEBUG_PRINT("axi bram: device class register");
    charClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(charClass))
    {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        DEBUG_PRINT("axi bram: fail to register device class");
        return PTR_ERR(charClass);
    }

    charDevice = device_create(charClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(charDevice))
    {
        class_destroy(charClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        DEBUG_PRINT("axi bram: fail to create the device");
        return PTR_ERR(charDevice);
    }
    DEBUG_PRINT("axi bram: device class create");
    DEBUG_PRINT("axi bram: driver load");

    return SUCCESS;
}


static void __exit mod_exit(void)
{
    iounmap(axibram_pointer);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    DEBUG_PRINT("axi bram: exit %s module", DEVICE_NAME);
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_AUTHOR("Nash");
MODULE_DESCRIPTION("Testdirver for AXI BRAM");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("custom:axi_bram");
