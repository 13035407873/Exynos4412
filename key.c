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

#define DEVICE_NAME "charDeviceKey" /* �������� */
#define DEVICE_CNT  1    /* ���豸�ŷ�Χ */

static dev_t deviceNum;    /* �豸�� */
static int major = 0;	   /* ���豸�� */
static int minor = 0;      /* ���豸�� */
static struct cdev key_cdev;       /* �ַ��豸�ṹ�� */
static struct class *key_class;    /* �࣬���������豸�ڵ� */
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
	.owner = THIS_MODULE, /* ����һ���꣬�������ģ��ʱ�Զ�������__this_module���� */
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


/* ģ����غ��� */
static  int key_init(void)
{
	int result,i;

	//��ʼ����ʱ�� key_timer
	init_timer(&key_timer);
	key_timer.function = key_timer_function;
	key_timer.expires = 0;
	add_timer(&key_timer);

	alloc_chrdev_region(&deviceNum, minor, DEVICE_CNT, DEVICE_NAME); /* ��̬�����豸�� */
	major = MAJOR(deviceNum);    
	minor = MINOR(deviceNum);

	cdev_init(&key_cdev, &key_fops);   			 /* �ַ��豸��ʼ�� */
	cdev_add(&key_cdev, deviceNum, DEVICE_CNT);  /* �����ַ��豸 */

	//���ں��Լ������豸�ڵ�  /dev/DEVICE_NAME
	key_class = class_create(THIS_MODULE,"keyChar");
	device_create(key_class, NULL, MKDEV(major, minor), NULL, DEVICE_NAME);

	for(i = 0; (pins_desc[i].pin != 0) || (pins_desc[i].key_val != 0); i++)
	{
		result = gpio_request(pins_desc[i].pin, pins_desc[i].name);   /* ����GPIO */
		printk("Request %s GPIO return %d\n", pins_desc[i].name, result);
		
		s3c_gpio_cfgpin(pins_desc[i].pin, S3C_GPIO_SFN(0xF));
		s3c_gpio_setpull(pins_desc[i].pin, S3C_GPIO_PULL_UP);

		result = request_irq(pins_desc[i].eint, Key_handler, IRQ_TYPE_EDGE_BOTH, pins_desc[i].name, &pins_desc[i]);

		printk("Request %s return %d\n", pins_desc[i].name, result);
	}
	
	return 0;
}

/* ģ��ж�غ��� */
static void key_exit(void)
{
	int i;
	del_timer(&key_timer);
	
	for(i = 0; (pins_desc[i].pin != 0) || (pins_desc[i].key_val != 0); i++)
	{
		free_irq(pins_desc[i].eint, &pins_desc[i]);
		gpio_free(pins_desc[i].pin);    /* �ͷ�GPIO */
	}
	
	device_destroy(key_class, MKDEV(major, minor));
	class_destroy(key_class);
	
	cdev_del(&key_cdev);    /* ɾ���ַ��豸 */
	unregister_chrdev_region(deviceNum, DEVICE_CNT);  /* �ͷ�������豸�� */
}

module_init(key_init);   /* ����ģ��ļ��غ��� */
module_exit(key_exit);   /* ����ģ���ж�غ��� */

MODULE_LICENSE("GPL");           /* �����������ص�Э�� */
MODULE_AUTHOR("Zhu Hong Chen");  /* �������������� */
