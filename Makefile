KVERS=$(shell uname -r)

obj-m += shn-cdev.o

kernel_modules:
	@$(MAKE) -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules

clean:
	@$(MAKE) -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean
	
