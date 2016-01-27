#!/bin/bash

#exit status
#1: run as non-root user
#2: sysfs/debugfs not mount
#3: insert module fail when gpio-mockup is a module.
#4: other reason.

module="gpio-mockup"

SYSFS=
GPIO_SYSFS=
GPIO_DRV_SYSFS=
DEBUGFS=
GPIO_DEBUGFS=
IS_CHARDEV=true

prerequisite()
{
	msg="skip all tests:"

	if [ $UID != 0 ]; then
		echo $msg must be run as root >&2
		exit 1
	fi

	SYSFS=`mount -t sysfs | head -1 | awk '{ print $3 }'`
	if [ ! -d "$SYSFS" ]; then
		echo $msg sysfs is not mounted >&2
		exit 2
	fi
	GPIO_SYSFS=`echo $SYSFS/class/gpio`
	GPIO_DRV_SYSFS=`echo $SYSFS/devices/platform/$module/gpio`

	DEBUGFS=`mount -t debugfs | head -1 | awk '{ print $3 }'`
	if [ ! -d "$DEBUGFS" ]; then
		echo $msg debugfs is not mounted >&2
		exit 2
	fi
	GPIO_DEBUGFS=`echo $DEBUGFS/gpio`

	source gpio-mockup-sysfs.sh
}

try_insert_module()
{
	if ![ -d "GPIO_DRV_SYSFS" ]; then
		modprobe -q $module $1
		if [ X$? != X0 ]; then
			echo $msg insmod $module failed >&2
			exit 3
		fi
	fi
}

remove_module()
{
	modprobe -r -q $module
}

die()
{
	remove_module
	exit 4
}

list_chip()
{
	name=$1
	if [ X$IS_CHARDEV = Xtrue ]; then
		#FIXME: gpio chardev could not check for specific gpio driver.
		gc=`lsgpio`
		if [ X"$gc" = X ]; then
			IS_CHARDEV=false
		else
			echo $gc
			return
		fi
	fi

	if [ X$IS_CHARDEV != Xtrue ]; then
		gc=`list_chip_sysfs $name`
	fi
	echo $gc
}

test_chip()
{
	if [ X$IS_CHARDEV != Xtrue ]; then
		test_chip_sysfs $*
	fi
}

gpio_test()
{
	param=$1
	valid=$2

	if [ X"$param" = X ]; then
		die
	fi
	try_insert_module "gpio_mockup_ranges=$param"
	echo -n "GPIO $module test with ranges configure<"
	echo "$param>: "
	printf "%-10s %s\n" $param
	gpiochip=`list_chip $module`
	if [ X"$gpiochip" = X ]; then
		if [ X"$valid" = Xtrue ]; then
			echo "successful"
		else
			echo "fail"
			die
		fi
	else
		for chip in $gpiochip; do
			test_chip $chip
		done
	fi
	remove_module
}

prerequisite

echo "1.  Do basic test: successful means insert gpiochip and manipulate gpio"
echo "    pin successful"
gpio_test "0,32" true
#gpio_test "0,32,32,16" true
#gpio_test "0,32,40,16,32,5" true
#echo "2.  Test dynamic allocation of gpio successful means insert gpiochip and"
#echo "    manipulate gpio pin successful"
#gpio_test "-1,32" true
#gpio_test "-1,32,32,16" true
#gpio_test "-1,32,40,16,-1,5" true
#echo "3.  Error test: successful means insert gpiochip failed"
#echo "3.1 Test zero line of gpio"
#gpio_test "0,0" false
#echo "3.2 Test range overlap"
#echo "3.2.1 Test corner case"
gpio_test "0,32,0,1" false
#gpio_test "0,32,32,5,32,10" false
#gpio_test "0,32,35,5,35,10" false
#gpio_test "0,32,31,1" false
#gpio_test "0,32,32,5,36,1" false
#gpio_test "0,32,35,5,34,2" false
#echo "3.2.2 Test inserting invalid second gpiochip"
#gpio_test "0,32,30,5" false
#gpio_test "0,32,1,5" false
#gpio_test "10,32,9,5" false
#gpio_test "10,32,30,5" false
#echo "3.2.3 Test others"
#gpio_test "0,32,40,16,39,5" false
#gpio_test "0,32,40,16,30,5" false
#gpio_test "0,32,40,16,30,11" false
#gpio_test "0,32,40,16,20,1" false

echo GPIO test PASS

