실행하는 법

1. git clone "this repo"
2. cd pintfs
3. make
4. gcc -o mkfs.pintfs mkfs.pintfs.c
5. dd if=/dev/zero of=pintdisk.raw bs=4k count=64 //256kb
6. sudo ./mkfs.pintfs pint_disk.raw
7. boot QEMU
8. sudo insmod pintfs.ko
9. sudo mount -o loop -t pintfs pintdisk.raw /mnt/pintfs