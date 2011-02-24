#!/bin/sh

#module="task_device"
#device="task_device"

/sbin/insmod ./spimod.ko 

#rm -f /dev/${device}
#major=$( awk  '$2 == d {print ($1)}' d=$device /proc/devices)
#echo "Major = $major"
#mknod /dev/${device}  c $major 0
#chmod a+rw /dev/task_device
