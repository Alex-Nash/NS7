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

#define AXI_BRAM_BASE 	0x40000000
#define AXI_BRAM_WIDTH	0x00001000


u32 *AxiBram_Pointer; 

#define DEVICE_NAME "axi_bram"
#define CLASS_NAME "bram"

#define SUCCESS 0

static int majorNumber;
static int Device_Open = 0;
static struct class*  charClass  = NULL;
static struct device* charDevice = NULL;


ssize_t bram_write(struct file *flip, const char *buffer, size_t length, loff_t *offset)
{
    u32 byte_write = 0;
    u32 tmpbuff[AXI_BRAM_WIDTH];

    if( copy_from_user((void*)AxiBram_Pointer, buffer, length))
        return -EINVAL;

    return SUCCESS;
}


ssize_t bram_read(struct file *flip, char *buffer, size_t length, loff_t *offset)
{
    if( copy_to_user(buffer, (void*)AxiBram_Pointer, length) )
        return -EINVAL;

    return SUCCESS;
}


static int bram_open(struct inode *inode, struct file *file)
{
    if (Device_Open)
        return -EBUSY;

    Device_Open++;
    printk("You tried to open the %s module.\n", DEVICE_NAME);
    try_module_get(THIS_MODULE);
    return SUCCESS;
}


static int bram_close(struct inode *inode, struct file *file)
{
    Device_Open--;

    module_put(THIS_MODULE);
    return SUCCESS;
}

static long bram_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct bram_rw_data data;

    if (copy_from_user(&data, (struct bram_rw_data*)arg, sizeof(struct bram_rw_data))) {
        DEBUG_PRINT("ioctl error getting arg\n");
        return -EACCES;
    }

    if(data.size > AXI_BRAM_WIDTH - data.offset) {
        DEBUG_PRINT("ioctl size more then memory width\n");
        return -EACCES;
    }

    u32 *value;

    value = kmalloc(data.size, GFP_KERNEL);

    switch (cmd) {
    case AXI_BRAM_WRITE:
        DEBUG_PRINT("ioctl write value");
        if (copy_from_user(value, (u32 *)data.data, data.size * sizeof(u32))) {
            DEBUG_PRINT("ioctl can't write from user");
            kfree(value);
            return -EACCES;
        }
        memcpy(AxiBram_Pointer + data.offset, value, data.size * sizeof(u32));
        DEBUG_PRINT("ioctl write ok!");
        break;
    case AXI_BRAM_READ:
        DEBUG_PRINT("ioctl read value");
        memcpy(value, AxiBram_Pointer + data.offset, data.size * sizeof(u32));

        if(copy_to_user(data.data, AxiBram_Pointer + data.offset, data.size * sizeof(u32))) {
            DEBUG_PRINT("ioctl can't copy to user");
            return -EACCES;
        }
        break;
    default:
        return -ENOTTY;
    }

    kfree(value);

    return 0;
}

struct file_operations fops = {
 //    .read = bram_read, 
 //    .write = bram_write, 
    .open = bram_open,
    .release = bram_close,
    .unlocked_ioctl = bram_ioctl
};


static int __init mod_init(void)
{
    printk(KERN_ERR "Init %s module. \n", DEVICE_NAME);
    AxiBram_Pointer = ioremap_nocache(AXI_BRAM_BASE, AXI_BRAM_WIDTH);

    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber<0){
        printk(KERN_ALERT "Axi-bram : failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "Axi-bram : Registered correctly with major number %d\n", majorNumber);

    charClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(charClass))
    {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Axi-bram : failed to register device class\n");
        return PTR_ERR(charClass);
    }
    printk(KERN_INFO "Axi-bram : device class registered correctly\n");

    charDevice = device_create(charClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(charDevice))
    {
        class_destroy(charClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Axi-bram : failed to create the device\n");
        return PTR_ERR(charDevice);
    }
    printk(KERN_INFO "Axi-bram : device class created correctly\n");
    printk(KERN_NOTICE "+ Axi-bram  Driver loading\n" );

    return SUCCESS;
}


static void __exit mod_exit(void)
{
    iounmap(AxiBram_Pointer);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_ERR "Exit %s Module. \n", DEVICE_NAME);
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_AUTHOR("Nash");
MODULE_DESCRIPTION("Testdirver for AXI BRAM");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("custom:axi_bram");
