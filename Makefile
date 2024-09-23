KDIR = /home/pyj1999/linux_study/linux

obj-m := pintfs.o

kbuild:
	make -C $(KDIR) M=`pwd`
clean:
	make -C $(KDIR) M=`pwd` clean

