obj-m += parport_pc.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
install:
	cp module_ch_dev.ko ../../RESULT
	cp install.sh ../../RESULT
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	
