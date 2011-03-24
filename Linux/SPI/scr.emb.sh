#!/bin/sh
# Installs all drivers and creates node spidev1.1 for spidev_test
module="spi"
device="spi"
deviceSP="spidev1.1"

modprobe spi_bitbang
/sbin/insmod ./spimod.ko 
modprobe spidev
rm -f /dev/${device}
major=$( awk  '$2 == d {print ($1)}' d=$device /proc/devices)
echo "Major = $major"
mknod /dev/${deviceSP}  c $major 0
chmod a+rw /dev/${deviceSP}
