/*
 * file: roulette.c
 *
 * Desc: A character device (/dev/roulette) that
 *      has a 1/6 chance of triggering a
 *      kernel panic when read.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A character device (/dev/roulette) that has a 1/6 chance of triggering a kernel panic when read.");
MODULE_AUTHOR("Korbin Marshall");
MODULE_VERSION("1.0");

#define DEVICE_NAME "roulettedev"
#define DRIVER_CLASS "rouletteclass"

#define STRING_WITH_LEN(literal) {.len = (sizeof(literal) - 1), .string = literal}

/*
 * Prototypes
 */
int init_module(void);
void cleanup_module(void);
static int dev_open(struct inode *inode, struct file *);
static int dev_release(struct inode *inode, struct file *);
static ssize_t dev_read(struct file *fp, char *buf, size_t len, loff_t *off);

/*
 * Our variables, @use_counter
 * will block concurrenty opens.
 * @buffer is the message and
 * @buffer_len the lenght of @buffer (duh)
 */
static char use_counter = 0;
static char buffer[512];
static int buffer_len = sizeof(buffer);
static u8 random_bullet;
static bool do_panic;
static bool show_warning;
static bool warning_over;

static dev_t dev;
static struct cdev *cdevp;
static struct class *roulette_class;


static struct file_operations fops = {
        .owner = THIS_MODULE,
        .read = dev_read,
        .open = dev_open,
        .release = dev_release
};

/*
 * Called when device is opened
 */
static int dev_open(struct inode *inode, struct file *fp) {
        if (use_counter)
                return -EBUSY;
        use_counter++;
        try_module_get(THIS_MODULE);
        // Spin the barrel
        get_random_bytes ( &random_bullet, sizeof (random_bullet) );
        random_bullet %= 6;
        return 0;
}

/*
 * Called when device is released. The device is
 * released when there is no process using it.
 */
static int dev_release(struct inode *inode, struct file *fp) {
        use_counter--;
        module_put(THIS_MODULE);
        if (warning_over)
                show_warning = false;
        if (do_panic) {
                mdelay(500);
                panic("Thanks for playing!");
        }
        return 0;
}


/*
 * @off controls
 * the "walk" through our buffer, is whith @off
 * that we say to user where is stoped.
 * @len is how much bytes to read. I almost ignore it.
 * I just check if is greater than 0.
 *
 * Called when device is read.
 * This method will read one, and only one byte per call,
 * If @off is longer than my buffer size or len is not
 * greater than 0 it returns 0, otherwise I copy one byte
 * to user buffer and returns the bytes readed, so 1.
 */
static ssize_t dev_read(struct file *fp, char *buf, size_t len, loff_t *off) {
        memset(buffer, 0, buffer_len); // Clear buffer
        if (show_warning) {
                sprintf(buffer,"\
WARNING, PLEASE READ!\n\
This module CAN AND WILL TRIGGER A KERNEL PANIC, and I HAVE NEVER SEEN IT SYNC.\n\
I AM NOT RESPONSIBLE IF YOUR DATA ENDS UP SCREWED BEYOND RECOVERY.\n\
IF YOU DON'T FEEL SAFE ANYMORE, THEN UNINSTALL THIS MODULE.\n\
THIS IS YOUR ONLY WARNING.\n\
USE AT YOUR OWN RISK.\n\
Otherwise, if you still want to play for some reason,\n\
then read this device again to immediately start the game.\n\
Also, this should hopefully be obvious, but please don't do this in real life.\n"
                );
                warning_over = true;
        } else if (random_bullet != 0) { // Pull the trigger
                sprintf(buffer, "Blank.\n");
        } else {
                sprintf(buffer, "BANG!\n");
                do_panic = true;
        }

        if (*off >= buffer_len || len <= 0)
                return 0;

        if (copy_to_user(buf, &buffer[*off], 1u))
                return -EFAULT;

        (*off)++;
        return 1;
}

// Kernel versions 6.2 and up take a const* to devices in dev_uevent
#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(6,2,0) )
typedef const struct device* uevent_dev_ptr;
#else
typedef struct device* uevent_dev_ptr;
#endif

// Make sure all users can thoroughly enjoy /dev/uwurandom
static int dev_uevent(uevent_dev_ptr dev, struct kobj_uevent_env* env) {
    int result = add_uevent_var(env, "DEVMODE=%#o", 0666);
    if (!result) {
        return result;
    }
    return 0;
}

/*
 * Called when module is load
 */
int modinit(void) {
        int error;

        /* Alloc a device region */
        error = alloc_chrdev_region(&dev, 1, 1, DEVICE_NAME);
        if (error)
                goto error_out;

        /* Registering */
        cdevp = cdev_alloc();
        if (!cdevp)
                return -ENOMEM;

        /* Allocate a device number */
        alloc_chrdev_region(&dev, 0, 1, "roulette");

        /* Create device class */
        #if ( LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0) )
        roulette_class = class_create(DRIVER_CLASS);
        #else
        roulette_class = class_create(THIS_MODULE, DRIVER_CLASS);
        #endif

        roulette_class->dev_uevent = dev_uevent;

        /* create device file */
        device_create(roulette_class, NULL, dev, NULL, "roulette");

        /* Init it! */
        cdev_init(cdevp, &fops);

        /* Tell the kernel "hey, I'm exist" */
        error = cdev_add(cdevp, dev, 1);
        if (error < 0)
                goto error_out;

        printk(KERN_INFO DEVICE_NAME " registred with major %d\n", MAJOR(dev));
        printk(KERN_INFO DEVICE_NAME " do: `mknod /dev/%s c %d %d' to create "
               "the device file\n",
               DEVICE_NAME, MAJOR(dev), MINOR(dev));
        show_warning = true;
        warning_over = false;
        return 0;

error_out:
        return -EFAULT;
}

void modexit(void) {
        device_destroy(roulette_class,dev);
        class_destroy(roulette_class);
        cdev_del(cdevp);
        unregister_chrdev_region(dev, 1);
}

module_init(modinit);
module_exit(modexit);
