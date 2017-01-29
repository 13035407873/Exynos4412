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
#include <linux/interrupt.h>
#include <linux/irq.h>

#define DEVICE_NAME "charDeviceKey" /* 驱动名称 */
#define DEVICE_CNT  1    /* 次设备号范围 */

static dev_t deviceNum;    /* 设备号 */
static int major = 0;	   /* 主设备号 */
static int minor = 0;      /* 次设备号 */
static struct cdev key_cdev;       /* 字符设备结构体 */
static struct class *key_class;    /* 类，用于生成设备节点 */
static struct timer_list key_timer;

struct pin_desc{
	unsigned int pin;
	unsigned int eint;
	unsigned int key_val;
	char *name;
};

static struct pin_desc pins_desc[6] = {
	{EXYNOS4_GPX1(1), IRQ_EINT(9) , 0x01, "KeyHome"},
	{EXYNOS4_GPX1(2), IRQ_EINT(10), 0x02, "KeyBack"},
	{EXYNOS4_GPX3(3), IRQ_EINT(27), 0x03, "KeySleep"},
	{EXYNOS4_GPX2(1), IRQ_EINT(17), 0x04, "KeyVol+"},
	{EXYNOS4_GPX2(0), IRQ_EINT(16), 0x05, "KeyVol-"},
	{0, 0, 0, "Stop"},
};

static struct pin_desc *pins;

static struct file_operations key_fops = {
	.owner = THIS_MODULE, /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
};

static irqreturn_t Key_handler(int irq, void *dev_id) 
{
	//printk("Key_handler!\n");
	//printk("irq is %d,dev is %d\n", irq, (int)dev_id);
	
	pins = (struct pin_desc *)dev_id;
	mod_timer(&key_timer,jiffies + HZ/100);
	return IRQ_HANDLED;
}

static void key_timer_function(unsigned long data)
{
	int val;
	if(pins == NULL)
		return;
	
	val = gpio_get_value(pins->pin);

	if(val == 0)
	{
		if((pins->key_val & 0x80) == 0x80)
			return;
		pins->key_val |= 0x80;
	}
	else
	{
		
		if((pins->key_val & 0x80) == 0)
			return;
		pins->key_val &= 0x7F;
	}
	printk("Key is 0x%02x\n", pins->key_val);
}


/* 模块加载函数 */
static  int key_init(void)
{
	int result,i;

	//初始化定时器 key_timer
	init_timer(&key_timer);
	key_timer.function = key_timer_function;
	key_timer.expires = 0;
	add_timer(&key_timer);

	alloc_chrdev_region(&deviceNum, minor, DEVICE_CNT, DEVICE_NAME); /* 动态申请设备号 */
	major = MAJOR(deviceNum);    
	minor = MINOR(deviceNum);

	cdev_init(&key_cdev, &key_fops);   			 /* 字符设备初始化 */
	cdev_add(&key_cdev, deviceNum, DEVICE_CNT);  /* 加载字符设备 */

	//让内核自己创建设备节点  /dev/DEVICE_NAME
	key_class = class_create(THIS_MODULE,"keyChar");
	device_create(key_class, NULL, MKDEV(major, minor), NULL, DEVICE_NAME);

	for(i = 0; (pins_desc[i].pin != 0) || (pins_desc[i].key_val != 0); i++)
	{
		result = gpio_request(pins_desc[i].pin, pins_desc[i].name);   /* 申请GPIO */
		printk("Request %s GPIO return %d\n", pins_desc[i].name, result);
		
		s3c_gpio_cfgpin(pins_desc[i].pin, S3C_GPIO_SFN(0xF));
		s3c_gpio_setpull(pins_desc[i].pin, S3C_GPIO_PULL_UP);

		result = request_irq(pins_desc[i].eint, Key_handler, IRQ_TYPE_EDGE_BOTH, pins_desc[i].name, &pins_desc[i]);

		printk("Request %s return %d\n", pins_desc[i].name, result);
	}
	
	return 0;
}

/* 模块卸载函数 */
static void key_exit(void)
{
	int i;
	del_timer(&key_timer);
	
	for(i = 0; (pins_desc[i].pin != 0) || (pins_desc[i].key_val != 0); i++)
	{
		free_irq(pins_desc[i].eint, &pins_desc[i]);
		gpio_free(pins_desc[i].pin);    /* 释放GPIO */
	}
	
	device_destroy(key_class, MKDEV(major, minor));
	class_destroy(key_class);
	
	cdev_del(&key_cdev);    /* 删除字符设备 */
	unregister_chrdev_region(deviceNum, DEVICE_CNT);  /* 释放申请的设备号 */
}

module_init(key_init);   /* 声明模块的加载函数 */
module_exit(key_exit);   /* 声明模块的卸载函数 */

MODULE_LICENSE("GPL");           /* 声明驱动遵守的协议 */
MODULE_AUTHOR("Zhu Hong Chen");  /* 声明驱动的作者 */
