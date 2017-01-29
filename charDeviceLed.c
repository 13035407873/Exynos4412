#include <linux/init.h>
#include <linux/module.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/gpio.h>
#include <mach/gpio-exynos4.h>

#define DEVICE_NAME "charDeviceLed" /* 驱动名称 */
#define DEVICE_CNT  1    /* 次设备号范围 */

static dev_t deviceNum;    /* 设备号 */
static int major = 0;	   /* 主设备号 */
static int minor = 0;      /* 次设备号 */
static struct cdev led_cdev;       /* 字符设备结构体 */
static struct class *led_class;    /* 类，用于生成设备节点 */

static int charDeviceLed_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int charDeviceLed_release(struct inode *inode,struct file *file)
{
	return 0;
}

static long charDeviceLed_ioctl(struct file *file, unsigned int cmd, unsigned long data)
{
	// cmd  0:LED1   1:LED2
	// data 0:低     1:高
	if((cmd==0 || cmd==1) && (data==0 || data==1))
	{
		if(cmd == 0)
		{
			//LED1
			if(data == 0)
				gpio_set_value(EXYNOS4_GPL2(0), 0);
			else
				gpio_set_value(EXYNOS4_GPL2(0), 1);
		}
		else
		{
			//LED2
			if(data == 0)
				gpio_set_value(EXYNOS4_GPK1(1), 0);
			else
				gpio_set_value(EXYNOS4_GPK1(1), 1);
		}
	}
	else
	{
		printk("CXLEDS: ioctl error!\n");
	}
	return 0;
}


static struct file_operations led_fops = {
	.owner = THIS_MODULE, /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
	.open           = charDeviceLed_open,
	.release        = charDeviceLed_release,
	.unlocked_ioctl = charDeviceLed_ioctl,   /* 新版Linux用unlocked_ioctl，而不是ioctl */
};

/* 模块加载函数 */
static  int led_init(void)
{
	alloc_chrdev_region(&deviceNum, minor, DEVICE_CNT, DEVICE_NAME); /* 动态申请设备号 */
	major = MAJOR(deviceNum);    
	minor = MINOR(deviceNum);

	gpio_request(EXYNOS4_GPL2(0),"LED1");   /* 申请GPIO */
	gpio_request(EXYNOS4_GPK1(1),"LED2");
	gpio_set_value(EXYNOS4_GPL2(0), 1);      /* GPIO置位 */
	gpio_set_value(EXYNOS4_GPK1(1), 1);      /* GPIO置位 */
	
	cdev_init(&led_cdev, &led_fops);   			 /* 字符设备初始化 */
	cdev_add(&led_cdev, deviceNum, DEVICE_CNT);  /* 加载字符设备 */

	//让内核自己创建设备节点  /dev/DEVICE_NAME
	led_class = class_create(THIS_MODULE,"ledChar");
	device_create(led_class, NULL, MKDEV(major, minor), NULL, DEVICE_NAME);
	
	return 0;
}

/* 模块卸载函数 */
static void led_exit(void)
{
	gpio_set_value(EXYNOS4_GPL2(0), 0);  /* GPIO清零 */
	gpio_set_value(EXYNOS4_GPK1(1), 0);  /* GPIO清零 */

	gpio_free(EXYNOS4_GPL2(0));    /* 释放GPIO */
	gpio_free(EXYNOS4_GPK1(1));    /* 释放GPIO */
	
	device_destroy(led_class, MKDEV(major, minor));
	class_destroy(led_class);
	
	cdev_del(&led_cdev);    /* 删除字符设备 */
	unregister_chrdev_region(deviceNum, DEVICE_CNT);  /* 释放申请的设备号 */
}

module_init(led_init);   /* 声明模块的加载函数 */
module_exit(led_exit);   /* 声明模块的卸载函数 */

MODULE_LICENSE("GPL");           /* 声明驱动遵守的协议 */
MODULE_AUTHOR("Zhu Hong Chen");  /* 声明驱动的作者 */

