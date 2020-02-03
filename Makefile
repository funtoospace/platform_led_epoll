KERN_DIR = /home/book/workspace/linux-2.6.22.6_jz

all:
	make -C $(KERN_DIR) M=`pwd` modules 

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order

obj-m	+= platform_led_dev.o platform_led_drv.o
