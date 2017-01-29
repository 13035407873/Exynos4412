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

#define DEVICE_NAME "tools" /* �������� */
#define DEVICE_CNT  1    /* ���豸�ŷ�Χ */

static dev_t deviceNum;    /* �豸�� */
static int major = 0;	   /* ���豸�� */
static int minor = 0;      /* ���豸�� */
static struct cdev tools_cdev;       /* �ַ��豸�ṹ�� */
static struct class *tools_class;    /* �࣬���������豸�ڵ� */

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
	gpio_request(EXYNOS4_GPL2(0),"LED1");	 /* ����GPIO */
	gpio_request(EXYNOS4_GPK1(1),"LED2");	 /* ����GPIO */
	s3c_gpio_cfgpin(EXYNOS4_GPL2(0), S3C_GPIO_SFN(0x01));
	s3c_gpio_cfgpin(EXYNOS4_GPK1(1), S3C_GPIO_SFN(0x01));
	gpio_set_value(EXYNOS4_GPL2(0), 0); 	
	gpio_set_value(EXYNOS4_GPK1(1), 0); 
	
	/* ADC */
	exynos_adc_set_channel(0);

	/* BEEP */
	gpio_request(EXYNOS4_GPD0(0),"BEEP");	 /* ����GPIO */
	s3c_gpio_cfgpin(EXYNOS4_GPD0(0), S3C_GPIO_SFN(0x01));
	gpio_set_value(EXYNOS4_GPD0(0), 0);
	//PWM����Ļ���ȿ��Ƶ�PWM��ͻ
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
	gpio_set_value(EXYNOS4_GPL2(0), 0);  /* GPIO���� */
	gpio_set_value(EXYNOS4_GPK1(1), 0);  /* GPIO���� */
	gpio_free(EXYNOS4_GPL2(0));    /* �ͷ�GPIO */
	gpio_free(EXYNOS4_GPK1(1));    /* �ͷ�GPIO */

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
	.owner = THIS_MODULE, /* ����һ���꣬�������ģ��ʱ�Զ�������__this_module���� */
	.open           = tools_open,
	.release        = tools_release,
	.unlocked_ioctl = tools_ioctl,
};

static int tools_probe(struct platform_device *pdev)
{
	printk("tools_probe!\n");
	
	alloc_chrdev_region(&deviceNum, minor, DEVICE_CNT, DEVICE_NAME); /* ��̬�����豸�� */
	major = MAJOR(deviceNum);    
	minor = MINOR(deviceNum);

	cdev_init(&tools_cdev, &tools_fops);   			 /* �ַ��豸��ʼ�� */
	cdev_add(&tools_cdev, deviceNum, DEVICE_CNT);  /* �����ַ��豸 */

	//���ں��Լ������豸�ڵ�  /dev/DEVICE_NAME
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
	
	cdev_del(&tools_cdev);    /* ɾ���ַ��豸 */
	unregister_chrdev_region(deviceNum, DEVICE_CNT);  /* �ͷ�������豸�� */
	
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


/* ģ����غ��� */
static  int tools_init(void)
{
	platform_driver_register(&tools_driver);	
	return 0;
}

/* ģ��ж�غ��� */
static void tools_exit(void)
{
	platform_driver_unregister(&tools_driver);
}

module_init(tools_init);   /* ����ģ��ļ��غ��� */
module_exit(tools_exit);   /* ����ģ���ж�غ��� */

MODULE_LICENSE("GPL");           /* �����������ص�Э�� */
MODULE_AUTHOR("Zhu Hong Chen");  /* �������������� */


