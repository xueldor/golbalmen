KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

#Kernel modules
obj-m += globalmem.o

build:kernel_modules

kernel_modules:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean: 
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.symvers *.order
