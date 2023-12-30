/*
 * file: korbo.c
 *
 * Desc: A simple device that
 *      only returns the string "Korbo".
 *
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/version.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple device that only returns the string \"Korbo\".");
MODULE_AUTHOR("Korbin Marshall");
MODULE_VERSION("1.0");

#define DEVICE_NAME "korbodev"
#define DRIVER_CLASS "korboclass"

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
static char buffer[] = "Korbo\n";
static int buffer_len = sizeof(buffer);

static dev_t dev;
static struct cdev *cdevp;
static struct class *korbo_class;


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
        return 0;
}

/*
 * Called when device is released. The device is
 * released when there is no process using it.
 */
static int dev_release(struct inode *inode, struct file *fp) {
        use_counter--;
        module_put(THIS_MODULE);
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
        alloc_chrdev_region(&dev, 0, 1, "korbo");

        /* Create device class */
        #if ( LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0) )
        korbo_class = class_create(DRIVER_CLASS);
        #else
        korbo_class = class_create(THIS_MODULE, DRIVER_CLASS);
        #endif

        korbo_class->dev_uevent = dev_uevent;

        /* create device file */
        device_create(korbo_class, NULL, dev, NULL, "korbo");

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

        return 0;

error_out:
        return -EFAULT;
}

void modexit(void) {
        device_destroy(korbo_class,dev);
        class_destroy(korbo_class);
        cdev_del(cdevp);
        unregister_chrdev_region(dev, 1);
        printk("cya --Korbo");
}

module_init(modinit);
module_exit(modexit);
