#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <asm/io.h>

struct itop4412_gpio_regs {
	unsigned long GPIOCON;
	unsigned long GPIODAT;
	unsigned long GPIOPUD;
	unsigned long GPIODRV;
	unsigned long GPIOCONPDN;
	unsigned long GPIOPUDPDN;
};

static struct itop4412_gpio_regs *led1_regs;
static struct itop4412_gpio_regs *led2_regs;

static int cxleds_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int cxleds_release(struct inode *inode,struct file *file)
{
	return 0;
}

static long cxleds_ioctl(struct file *file, unsigned int cmd, unsigned long data)
{
	// cmd  0:LED1   1:LED2
	// data 0:低     1:高
	if((cmd==0 || cmd==1) && (data==0 || data==1))
	{
		if(cmd == 0)
		{
			//LED1
			if(data == 0)
			{
				//低电平
				led1_regs->GPIODAT &= 0xFE;  //GPL2_0 低电平
			}
			else
			{
				//高电平
				led1_regs->GPIODAT |= 0x01; 
			}
		}
		else
		{
			//LED2
			if(data == 0)
			{
				//低电平
				led2_regs->GPIODAT &= 0xFD;  //GPK1_1 低电平
			}
			else
			{
				//高电平
				led2_regs->GPIODAT |= 0x02; 
			}
		}
	}
	else
	{
		printk("CXLEDS: ioctl error!\n");
	}
	return 0;
}

static struct file_operations cxleds_ops = {
	.owner          = THIS_MODULE,
	.open           = cxleds_open,
	.release        = cxleds_release,
	.unlocked_ioctl = cxleds_ioctl,
};


static struct miscdevice misc_cxleds = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "cxledsDriver",
	.fops  = &cxleds_ops,
};

static int cxleds_probe(struct platform_device *pdev)
{
	misc_register(&misc_cxleds);
	return 0;
}

static int cxleds_remove(struct platform_device *pdev)
{
	misc_deregister(&misc_cxleds);
	return 0;
}

static void cxleds_shutdown(struct platform_device *pdev)
{

}


static struct platform_driver cxleds_driver = {
	.probe    = cxleds_probe,
	.remove   = cxleds_remove,
	.shutdown = cxleds_shutdown,
	.driver   = {
		.name  = "cxleds",
		.owner = THIS_MODULE,
	},
};

static int  cxleds_init(void)
{
	platform_driver_register(&cxleds_driver);
	led1_regs = ioremap(0x11000100,sizeof(struct itop4412_gpio_regs));
	led2_regs = ioremap(0x11000060,sizeof(struct itop4412_gpio_regs));

	//初始化LED1
	led1_regs->GPIOCON &= 0xFFFFFFF0;
	led1_regs->GPIOCON |= 0x00000001;

	led1_regs->GPIODAT &= 0xFE;  //GPL2_0 低电平

	//初始化LED2
	led2_regs->GPIOCON &= 0xFFFFFF0F;
	led2_regs->GPIOCON |= 0x00000010;

	led2_regs->GPIODAT &= 0xFD;  //GPK1_1 低电平
	
	return 0;
}

static void cxleds_exit(void)
{
	iounmap(led1_regs);
	iounmap(led2_regs);
	platform_driver_unregister(&cxleds_driver);
}

module_init(cxleds_init);
module_exit(cxleds_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zhu Hong Chen");

