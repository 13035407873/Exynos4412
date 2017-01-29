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
#include <asm/io.h>
#include <linux/platform_device.h>
#include <plat/adc.h>

#define DEVICE_NAME "tools" /* 驱动名称 */
#define DEVICE_CNT  1    /* 次设备号范围 */

static dev_t deviceNum;    /* 设备号 */
static int major = 0;	   /* 主设备号 */
static int minor = 0;      /* 次设备号 */
static struct cdev tools_cdev;       /* 字符设备结构体 */
static struct class *tools_class;    /* 类，用于生成设备节点 */

/*
struct pwm_register {
	unsigned long TCFG0;
	unsigned long TCFG1;
	unsigned long TCON;
	unsigned long TCNTB0;
	unsigned long TCMPB0;
	unsigned long TCNTO0;
};

static struct pwm_register *pwm_beep;
*/

typedef struct {
	struct mutex lock;
	struct s3c_adc_client *client;
	int channel;
} ADC_DEV;

static ADC_DEV adcdev;

static int exynos_adc_read_ch(void) 
{
	int ret;

	ret = mutex_lock_interruptible(&adcdev.lock);
	if (ret < 0)
		return ret;

	ret = s3c_adc_read(adcdev.client, adcdev.channel);
	mutex_unlock(&adcdev.lock);

	return ret;
}

static void exynos_adc_set_channel(int channel) 
{
	if (channel < 0 || channel > 3)
		return;

	adcdev.channel = channel;
}


static int tools_open(struct inode *inode, struct file *file)
{
	printk("tools_open\n");

	/* LED */
	gpio_request(EXYNOS4_GPL2(0),"LED1");	 /* 申请GPIO */
	gpio_request(EXYNOS4_GPK1(1),"LED2");	 /* 申请GPIO */
	s3c_gpio_cfgpin(EXYNOS4_GPL2(0), S3C_GPIO_SFN(0x01));
	s3c_gpio_cfgpin(EXYNOS4_GPK1(1), S3C_GPIO_SFN(0x01));
	gpio_set_value(EXYNOS4_GPL2(0), 0); 	
	gpio_set_value(EXYNOS4_GPK1(1), 0); 
	
	/* ADC */
	exynos_adc_set_channel(0);

	/* BEEP */
	gpio_request(EXYNOS4_GPD0(0),"BEEP");	 /* 申请GPIO */
	s3c_gpio_cfgpin(EXYNOS4_GPD0(0), S3C_GPIO_SFN(0x01));
	gpio_set_value(EXYNOS4_GPD0(0), 0);
	//PWM与屏幕亮度控制的PWM冲突
	//pwm_beep = ioremap(0x139D0000,sizeof(struct pwm_register));

	//pwm_beep->TCFG0 &= 0xFFFFFF00;
	//pwm_beep->TCFG0 |= (50 - 1);
	//pwm_beep->TCFG1 &= 0xFFFFFFF0;
	//pwm_beep->TCFG1 |= 0x04;
	//pwm_beep->TCNTB0 = 100;
	//pwm_beep->TCMPB0 = 100;
	//pwm_beep->TCON &= 0xFFFFFFE0;
	//pwm_beep->TCON = pwm_beep->TCON | (1<<0) | (1<<1) | (1<<2);
	
	return 0;
}


/*
static void beep(unsigned long data)
{
	data = (data >= 100 ? 100 : data);
	printk("Beep: %ld\n", data);
	if(data == 0)
	{
		pwm_beep->TCMPB0 = 100;
		pwm_beep->TCON &= 0xFFFFFFFE;
	}
	else
	{
		pwm_beep->TCMPB0 = 100 - data;
		pwm_beep->TCON &= 0xFFFFFFE0;
		pwm_beep->TCON = pwm_beep->TCON | (1<<0) | (1<<2) | (1<<3);
	}
}
*/

static int tools_release(struct inode *inode,struct file *file)
{
	printk("tools_release\n");

	/* LED */
	gpio_set_value(EXYNOS4_GPL2(0), 0);  /* GPIO清零 */
	gpio_set_value(EXYNOS4_GPK1(1), 0);  /* GPIO清零 */
	gpio_free(EXYNOS4_GPL2(0));    /* 释放GPIO */
	gpio_free(EXYNOS4_GPK1(1));    /* 释放GPIO */

	/* BEEP */
	//iounmap(pwm_beep);
	//s3c_gpio_cfgpin(EXYNOS4_GPD0(0), S3C_GPIO_SFN(0x01));
	gpio_set_value(EXYNOS4_GPD0(0), 0);
	gpio_free(EXYNOS4_GPD0(0));

	/* ADC */

	
	return 0;
}

static long tools_ioctl(struct file *file, unsigned int cmd, unsigned long data)
{
	cmd = _IOC_NR(cmd);
	printk("tools_ioctl: %d %ld\n", cmd, data);
	
	if(cmd == 0)
	{
		//LED1
		if(data == 0)
			gpio_set_value(EXYNOS4_GPL2(0), 0);
		else
			gpio_set_value(EXYNOS4_GPL2(0), 1);
		return 0;
	}
	else if(cmd == 1)
	{
		//LED2
		if(data == 0)
			gpio_set_value(EXYNOS4_GPK1(1), 0);
		else
			gpio_set_value(EXYNOS4_GPK1(1), 1);
		return 0;
	}
	else if(cmd == 2)
	{
		//BEEP
		//beep(data);
		if(data == 0)
			gpio_set_value(EXYNOS4_GPD0(0), 0);
		else
			gpio_set_value(EXYNOS4_GPD0(0), 1);
		return 0;
	}
	else if(cmd == 3)
	{
		//ADC
		return exynos_adc_read_ch();
	}
	
	return -1;
}

static struct file_operations tools_fops = {
	.owner = THIS_MODULE, /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
	.open           = tools_open,
	.release        = tools_release,
	.unlocked_ioctl = tools_ioctl,
};

static int tools_probe(struct platform_device *pdev)
{
	printk("tools_probe!\n");
	
	alloc_chrdev_region(&deviceNum, minor, DEVICE_CNT, DEVICE_NAME); /* 动态申请设备号 */
	major = MAJOR(deviceNum);    
	minor = MINOR(deviceNum);

	cdev_init(&tools_cdev, &tools_fops);   			 /* 字符设备初始化 */
	cdev_add(&tools_cdev, deviceNum, DEVICE_CNT);  /* 加载字符设备 */

	//让内核自己创建设备节点  /dev/DEVICE_NAME
	tools_class = class_create(THIS_MODULE,"tools");
	device_create(tools_class, NULL, MKDEV(major, minor), NULL, DEVICE_NAME);

	//ADC
	mutex_init(&adcdev.lock);
	adcdev.client = s3c_adc_register(pdev, NULL, NULL, 0);
	
	return 0;
}

static int tools_remove(struct platform_device *pdev)
{
	s3c_adc_release(adcdev.client);

	device_destroy(tools_class, MKDEV(major, minor));
	class_destroy(tools_class);
	
	cdev_del(&tools_cdev);    /* 删除字符设备 */
	unregister_chrdev_region(deviceNum, DEVICE_CNT);  /* 释放申请的设备号 */
	
	return 0;
}

static void tools_shutdown(struct platform_device *pdev)
{

}

static struct platform_driver tools_driver = {
	.probe    = tools_probe,
	.remove   = tools_remove,
	.shutdown = tools_shutdown,
	.driver   = {
		.name  = "tools",
		.owner = THIS_MODULE,
	},
};


/* 模块加载函数 */
static  int tools_init(void)
{
	platform_driver_register(&tools_driver);	
	return 0;
}

/* 模块卸载函数 */
static void tools_exit(void)
{
	platform_driver_unregister(&tools_driver);
}

module_init(tools_init);   /* 声明模块的加载函数 */
module_exit(tools_exit);   /* 声明模块的卸载函数 */

MODULE_LICENSE("GPL");           /* 声明驱动遵守的协议 */
MODULE_AUTHOR("Zhu Hong Chen");  /* 声明驱动的作者 */


