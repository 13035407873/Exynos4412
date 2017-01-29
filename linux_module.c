#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

static int hello_open(struct inode *inode, struct file *file)
{
	printk("hello_open\n");
	return 0;
}

static int hello_release(struct inode *inode,struct file *file)
{
	printk("hello_release\n");
	return 0;
}

static long hello_ioctl(struct file *file, unsigned int cmd, unsigned long data)
{
	printk("hello_ioctl: %d %ld\n", cmd, data);
	return 0;
}

static struct file_operations hello_ops = {
	.owner          = THIS_MODULE,
	.open           = hello_open,
	.release        = hello_release,
	.unlocked_ioctl = hello_ioctl,
};

static struct miscdevice misc_hello = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "helloDriver",
	.fops  = &hello_ops,
};

static int hello_probe(struct platform_device *pdev)
{
	printk("hello_probe\n");
	misc_register(&misc_hello);
	return 0;
}

static int hello_remove(struct platform_device *pdev)
{
	printk("hello_remove\n");
	misc_deregister(&misc_hello);
	return 0;
}

static void hello_shutdown(struct platform_device *pdev)
{
	printk("hello_shutdown\n");
}

static struct platform_driver hello_driver = {
	.probe    = hello_probe,
	.remove   = hello_remove,
	.shutdown = hello_shutdown,
	.driver   = {
		.name  = "hello",
		.owner = THIS_MODULE,
	},
};

static int  hello_init(void)
{
	printk("module_init\n");
	platform_driver_register(&hello_driver);
	return 0;
}

static void hello_exit(void)
{
	printk("module_exit\n");
	platform_driver_unregister(&hello_driver);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zhu Hong Chen");

