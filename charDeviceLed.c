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

#define DEVICE_NAME "charDeviceLed" /* �������� */
#define DEVICE_CNT  1    /* ���豸�ŷ�Χ */

static dev_t deviceNum;    /* �豸�� */
static int major = 0;	   /* ���豸�� */
static int minor = 0;      /* ���豸�� */
static struct cdev led_cdev;       /* �ַ��豸�ṹ�� */
static struct class *led_class;    /* �࣬���������豸�ڵ� */

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
	// data 0:��     1:��
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
	.owner = THIS_MODULE, /* ����һ���꣬�������ģ��ʱ�Զ�������__this_module���� */
	.open           = charDeviceLed_open,
	.release        = charDeviceLed_release,
	.unlocked_ioctl = charDeviceLed_ioctl,   /* �°�Linux��unlocked_ioctl��������ioctl */
};

/* ģ����غ��� */
static  int led_init(void)
{
	alloc_chrdev_region(&deviceNum, minor, DEVICE_CNT, DEVICE_NAME); /* ��̬�����豸�� */
	major = MAJOR(deviceNum);    
	minor = MINOR(deviceNum);

	gpio_request(EXYNOS4_GPL2(0),"LED1");   /* ����GPIO */
	gpio_request(EXYNOS4_GPK1(1),"LED2");
	gpio_set_value(EXYNOS4_GPL2(0), 1);      /* GPIO��λ */
	gpio_set_value(EXYNOS4_GPK1(1), 1);      /* GPIO��λ */
	
	cdev_init(&led_cdev, &led_fops);   			 /* �ַ��豸��ʼ�� */
	cdev_add(&led_cdev, deviceNum, DEVICE_CNT);  /* �����ַ��豸 */

	//���ں��Լ������豸�ڵ�  /dev/DEVICE_NAME
	led_class = class_create(THIS_MODULE,"ledChar");
	device_create(led_class, NULL, MKDEV(major, minor), NULL, DEVICE_NAME);
	
	return 0;
}

/* ģ��ж�غ��� */
static void led_exit(void)
{
	gpio_set_value(EXYNOS4_GPL2(0), 0);  /* GPIO���� */
	gpio_set_value(EXYNOS4_GPK1(1), 0);  /* GPIO���� */

	gpio_free(EXYNOS4_GPL2(0));    /* �ͷ�GPIO */
	gpio_free(EXYNOS4_GPK1(1));    /* �ͷ�GPIO */
	
	device_destroy(led_class, MKDEV(major, minor));
	class_destroy(led_class);
	
	cdev_del(&led_cdev);    /* ɾ���ַ��豸 */
	unregister_chrdev_region(deviceNum, DEVICE_CNT);  /* �ͷ�������豸�� */
}

module_init(led_init);   /* ����ģ��ļ��غ��� */
module_exit(led_exit);   /* ����ģ���ж�غ��� */

MODULE_LICENSE("GPL");           /* �����������ص�Э�� */
MODULE_AUTHOR("Zhu Hong Chen");  /* �������������� */

