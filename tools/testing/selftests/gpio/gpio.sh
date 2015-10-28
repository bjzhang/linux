#!/bin/bash

#TODO do I need support other gpio driver?
module="gpio-mockup"

SYSFS=
DEBUGFS=
GPIO_DRV_SYSFS=

prerequisite()
{
	msg="skip all tests:"

	if [ $UID != 0 ]; then
		echo $msg must be run as root >&2
		exit 0
	fi

	SYSFS=`mount -t sysfs | head -1 | awk '{ print $3 }'`

	if [ ! -d "$SYSFS" ]; then
		echo $msg sysfs is not mounted >&2
		exit 0
	fi

	DEBUGFS=`mount -t debugfs | head -1 | awk '{ print $3 }'`
	if [ ! -d "$DEBUGFS" ]; then
		echo $msg debugfs is not mounted >&2
		exit 0
	fi

	if [ ! modprobe -r -q $module ]; then
		echo $msg insmod $module failed >&2
		exit 0
	fi

	echo "GPIO $module summary:"
	printf "%10s %5s %5s\n" name base ngpio
	GPIO_DRV_SYSFS=`echo /sys/devices/platform/$module/gpio`
	for chip in `ls -d $GPIO_DRV_SYSFS/*`; do
		name=`basename $chip`
		base=`cat $chip/base`
		ngpio=`cat $chip/base`
		printf "%10s %5s %5s\n" $name $base $ngpio
	done
}


