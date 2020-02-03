#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>

static struct platform_device *platform_led_pdev;

static int __init platform_led_dev_init(void)
{
	int ret;

	platform_led_pdev = platform_device_alloc("platform_led", -1);
	if (!platform_led_pdev)
		return -ENOMEM;

	ret = platform_device_add(platform_led_pdev);
	if (ret) {
		platform_device_put(platform_led_pdev);
		return ret;
	}


	printk(KERN_INFO "platform_led is created !!!\n");
	
	return 0;

}
module_init(platform_led_dev_init);

static void __exit platform_led_dev_exit(void)
{
	platform_device_unregister(platform_led_pdev);
}
module_exit(platform_led_dev_exit);

MODULE_AUTHOR("WL");
MODULE_LICENSE("GPL v2");
