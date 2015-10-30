#!/bin/bash

#TODO do I need support other gpio driver? It could be if then implement the 'standard' debugfs in gpiolib.
module="gpio-mockup"

SYSFS=
GPIO_SYSFS=
GPIO_DRV_SYSFS=
DEBUGFS=
GPIO_DEBUGFS=

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

	GPIO_SYSFS=`echo $SYSFS/class/gpio`

	DEBUGFS=`mount -t debugfs | head -1 | awk '{ print $3 }'`
	if [ ! -d "$DEBUGFS" ]; then
		echo $msg debugfs is not mounted >&2
		exit 0
	fi

	GPIO_DEBUGFS=`echo $DEBUGFS/gpio`

	if modprobe -q $module ; then
		echo -n ""
	else
		echo $msg insmod $module failed >&2
		exit 0
	fi

}

is_consistent()
{
	active_low_sysfs=`cat $GPIO_SYSFS/gpio$nr/active_low`
	val_sysfs=`cat $GPIO_SYSFS/gpio$nr/value`
	dir_sysfs=`cat $GPIO_SYSFS/gpio$nr/direction`

	gpio_this_debugfs=`cat $GPIO_DEBUGFS |grep "gpio-$nr" | sed "s/(.*)//g"`
	dir_debugfs=`echo $gpio_this_debugfs | awk '{print $2}'`
	val_debugfs=`echo $gpio_this_debugfs | awk '{print $3}'`
	if [ $val_debugfs = "lo" ]; then
		val_debugfs=0
	elif [ $val_debugfs = "hi" ]; then
		val_debugfs=1
	fi

	if [ $active_low_sysfs = "1" ]; then
		if [ $val_debugfs = "0" ]; then
			val_debugfs="1"
		else
			val_debugfs="0"
		fi
	fi

	if [ $val_sysfs = $val_debugfs ] && [ $dir_sysfs = $dir_debugfs ]; then
		echo "yes"
	else
		echo "no"
	fi
}

test_one_gpio()
{
	nr=$1
	direction=$2
	active_low=$3
	value=$4

	echo $direction > $GPIO_SYSFS/gpio$nr/direction
	echo $active_low > $GPIO_SYSFS/gpio$nr/active_low
    if [ $direction = "out" ]; then
        echo $value > $GPIO_SYSFS/gpio$nr/value
    fi
	is_consistent $nr
}

test_gpio()
{
	nr=$1

	echo $nr > $GPIO_SYSFS/export

	echo -n "Checking if the sysfs is consistent with debugfs: "
	is_consistent $nr

	echo -n "Checking the logic of active_low: "
	test_one_gpio $nr out 1 1
	test_one_gpio $nr out 1 0
	test_one_gpio $nr out 0 1
	test_one_gpio $nr out 0 0

	echo -n "Checking the logic of direction: "
	test_one_gpio $nr in 1 1
	test_one_gpio $nr out 1 0
	test_one_gpio $nr low 0 1
	test_one_gpio $nr high 0 0

	echo $nr > $GPIO_SYSFS/unexport
}

prerequisite

echo "GPIO $module summary:"
printf "%10s %5s %5s\n" name base ngpio
GPIO_DRV_SYSFS=`echo $SYSFS/devices/platform/$module/gpio`
for chip in `ls -d $GPIO_DRV_SYSFS/gpiochip*`; do
	name=`basename $chip`
	base=`cat $chip/base`
	ngpio=`cat $chip/ngpio`
	printf "%10s %5s %5s\n" $name $base $ngpio
	test_gpio $base
	test_gpio $(($base + $ngpio))
	test_gpio $((( RANDOM % $ngpio )  + $base ))

done

modprobe -r -q $module
