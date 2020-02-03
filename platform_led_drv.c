#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>

//static struct class *firstdrv_class;
//static struct class_device	*firstdrv_class_dev;

volatile unsigned long *gpfcon = NULL;
volatile unsigned long *gpfdat = NULL;

struct platform_led_dev {
	struct cdev cdev;
	unsigned int flag;
	struct mutex mutex;
	wait_queue_head_t r_wait;
//	wait_queue_head_t w_wait;
//	struct fasync_struct *async_queue;
	struct miscdevice miscdev;
};

struct platform_led_dev *pld;

static int platform_led_open(struct inode *inode, struct file *filp)
{
	filp->private_data = pld;

	printk("platform_led_open\n");
	/* 配置GPF4,5,6为输出 中文 */
	*gpfcon &= ~((0x3<<(4*2)) | (0x3<<(5*2)) | (0x3<<(6*2)));
	*gpfcon |= ((0x1<<(4*2)) | (0x1<<(5*2)) | (0x1<<(6*2)));
	printk("platform_led_open ok\n");
	return 0;
}

static int platform_led_poll(struct file *filp, poll_table * wait)
{
	unsigned int mask =0;

	struct platform_led_dev *dev = filp->private_data;

	mutex_lock(&dev->mutex);

	poll_wait(filp, &dev->r_wait, wait);

	if (dev->flag == 1) {
		mask |= POLLIN | POLLRDNORM;
		printk(KERN_INFO "mask is : %d\n", mask);
	}
	
	if (dev->flag == 0) {
		mask &= ~(POLL_IN | POLLRDNORM);
		printk(KERN_INFO "mask is : %d\n", mask);
	}

	mutex_unlock(&dev->mutex);
	return mask;
}

static ssize_t platform_led_write(struct file *filp, const char __user *buf, size_t count, loff_t * ppos)
{
	int val;
	//struct platform_led_dev *dev = container_of(filp->private_data, struct platform_led_dev, miscdev);
	struct platform_led_dev *dev = filp->private_data;
	printk(KERN_INFO "dev address:%d\n", dev);
	printk(KERN_INFO "pld address:%d\n", pld);
	
	int ret;
	 
//	DECLARE_WAITQUEUE(wait, current);
/*  
	mutex_lock(&dev->mutex);
	add_wait_queue(&dev->w_wait, &wait);

	//printk("first_drv_write\n");

	while (dev->flag == 0) {
		if (filp->f_flags & O_NONBLOCK){
			ret = -EAGAIN;
			goto out;
		}

		__set_current_state(TASK_INTERRUPTIBLE);
		
		mutex_unlock(&dev->mutex);

		schedule();

		if (signal_pending(current)){
			ret = -ERESTARTSYS;
			goto out2;
		}

		mutex_lock(&dev->mutex);
	}
*/
	printk(KERN_INFO "copy_from_user begin\n");
	copy_from_user(&val, buf, count); //	copy_to_user();
	printk(KERN_INFO "copy_from_user end\n");

	if (val == 1)
	{
		// 点灯
		*gpfdat &= ~((1<<4) | (1<<5) | (1<<6));
		dev->flag = 1;
		wake_up_interruptible(&dev->r_wait);
	}
	else
	{
		// 灭灯
		*gpfdat |= (1<<4) | (1<<5) | (1<<6);
		dev->flag = 0;
	}
	
	//wake_up_interruptible(&dev->r_wait);
/*  
	if (dev->async_queue){
		kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
		printk(KERN_DEBUG "%s kill SIGIO\n", __func__);
	}
*/
out:
	//mutex_unlock(&dev->mutex);
//out2:
//	remove_wait_queue(&dev->w_wait, &wait);
//	set_current_state(TASK_RUNNING);
	return 0;
}

static ssize_t platform_led_read(struct file *filp, char __user *buf, size_t count, loff_t * ppos)
{
	int val;
	//struct platform_led_dev *dev = container_of(filp->private_data, struct platform_led_dev, miscdev);
	struct platform_led_dev *dev = filp->private_data;

	int ret;
	  
	DECLARE_WAITQUEUE(wait, current);

	mutex_lock(&dev->mutex);
	add_wait_queue(&dev->r_wait, &wait);

	//printk("first_drv_write\n");
  
	while (dev->flag == 0) {
	// while (*gpfdat & ((1<<4) | (1<<5) | (1<<6))) {
		if (filp->f_flags & O_NONBLOCK){
			ret = -EAGAIN;
			goto out;
		}

		__set_current_state(TASK_INTERRUPTIBLE);
		
		mutex_unlock(&dev->mutex);

		schedule();

		if (signal_pending(current)){
			ret = -ERESTARTSYS;
			goto out2;
		}

		mutex_lock(&dev->mutex);
	}


	if (*gpfdat & ((1<<4) | (1<<5) | (1<<6)))
	{
		// 点灯
		val = 0;
		printk(KERN_INFO "\nval = %d\n", val);
	}
	else
	{
		// 灭灯
		val = 1;
		printk(KERN_INFO "\nval = %d\n", val);
	}
	
	printk(KERN_INFO "copy_to_user begin\n");
	copy_to_user(buf, &val, count); //	copy_to_user();
	printk(KERN_INFO "copy_to_user end\n");
	//wake_up_interruptible(&dev->r_wait);
/*  
	if (dev->async_queue){
		kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
		printk(KERN_DEBUG "%s kill SIGIO\n", __func__);
	}
*/
out:
	mutex_unlock(&dev->mutex);
out2:
	remove_wait_queue(&dev->r_wait, &wait);
	set_current_state(TASK_RUNNING);
	return 0;
}

static struct file_operations platform_led_fops = {
	.owner = THIS_MODULE,
	.open  = platform_led_open,
	.read  = platform_led_read,
	.write = platform_led_write,
	.poll = platform_led_poll,
	
};
/* 
static struct file_operations first_drv_fops = {
    .owner  =   THIS_MODULE,    // 这是一个宏，推向编译模块时自动创建的__this_module变量 
    .open   =   first_drv_open,     
	.write	=	first_drv_write,	   
};
*/


static int platform_led_probe(struct platform_device *pdev)
{
	//struct platform_led_dev *pl;
	int ret;

	pld = devm_kzalloc(&pdev->dev, sizeof(*pld), GFP_KERNEL);
	if(!pld)
		return -ENOMEM;
	pld->miscdev.minor = MISC_DYNAMIC_MINOR;
	pld->miscdev.name = "platform_led";
	pld->miscdev.fops = &platform_led_fops;

	mutex_init(&pld->mutex);
	init_waitqueue_head(&pld->r_wait);
//	init_waitqueue_head(&pld->w_wait);
	platform_set_drvdata(pdev, pld);

	ret = misc_register(&pld->miscdev);
	if (ret < 0)
		goto err;

	gpfcon = (volatile unsigned long *)ioremap(0x56000050, 16);
	gpfdat = gpfcon + 1;

	dev_info(&pdev->dev, "platform_led drv probed\n");

	return 0;
err:
	return ret;
}

static int platform_led_remove(struct platform_device *pdev)
{
	struct platform_led_dev *pl = platform_get_drvdata(pdev);

	misc_deregister(&pl->miscdev);

	dev_info(&pdev->dev, "platform_led dev removed\n");

	iounmap(gpfcon);

	return 0;
}
/*  
int major;
static int first_drv_init(void)
{
	major = register_chrdev(0, "first_drv", &first_drv_fops); // 注册, 告诉内核

	firstdrv_class = class_create(THIS_MODULE, "firstdrv");

	firstdrv_class_dev = class_device_create(firstdrv_class, NULL, MKDEV(major, 0), NULL, "xyz"); // /dev/xyz 

	gpfcon = (volatile unsigned long *)ioremap(0x56000050, 16);
	gpfdat = gpfcon + 1;

	return 0;
}
static void first_drv_exit(void)
{
	unregister_chrdev(major, "first_drv"); // 卸载

	class_device_unregister(firstdrv_class_dev);
	class_destroy(firstdrv_class);
	iounmap(gpfcon);
}

*/
static struct platform_driver platform_led_driver = {
	.driver = {
		.name = "platform_led",
		.bus = &platform_bus_type,
		.owner = THIS_MODULE,
	},
	.probe = platform_led_probe,
	.remove = platform_led_remove,
};

static int __init platform_led_init(void)
{
	return platform_driver_register(&platform_led_driver);
}
module_init(platform_led_init);

static int __exit platform_led_exit(void)
{
	platform_driver_unregister(&platform_led_driver);
}
module_exit(platform_led_exit);

// module_platform_driver(platform_led_driver);


//module_init(first_drv_init);
//module_exit(first_drv_exit);

MODULE_AUTHOR("WL");
MODULE_LICENSE("GPL");
