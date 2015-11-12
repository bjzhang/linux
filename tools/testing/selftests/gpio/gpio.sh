#!/bin/bash

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
	GPIO_DRV_SYSFS=`echo $SYSFS/devices/platform/$module/gpio`

	DEBUGFS=`mount -t debugfs | head -1 | awk '{ print $3 }'`
	if [ ! -d "$DEBUGFS" ]; then
		echo $msg debugfs is not mounted >&2
		exit 0
	fi
	GPIO_DEBUGFS=`echo $DEBUGFS/gpio`

}

insert_module()
{
	if modprobe -q $module $1; then
		echo -n ""
	else
		echo $msg insmod $module failed >&2
		exit 0
	fi
}

remove_module()
{
	modprobe -r -q $module
}

die()
{
	remove_module
	exit 1
}

gpio_test()
{
	param=$1
	if [ X$2 != X ]; then
		invalid_pin=$2
	fi
	if [ X$param = X ]; then
		insert_module
	else
		insert_module "conf=$param"
	fi

	echo -n "GPIO $module test with base,ngpio configure<"
	if [ X$param = X ]; then
		echo "(default)>: ";
	else
		echo "$param>: "
	fi
	printf "%-10s %-5s %-5s\n" name base ngpio
	gpiochip=`ls -d $GPIO_DRV_SYSFS/gpiochip* 2>/dev/null`
	if [ X"$gpiochip" = X ]; then
		echo "no gpiochip"
	else
		for chip in $gpiochip; do
			name=`basename $chip`
			base=`cat $chip/base`
			ngpio=`cat $chip/ngpio`
			printf "%-10s %-5s %-5s\n" $name $base $ngpio
			if [ $ngpio = "0" ]; then
				echo "number of gpio is zero is not allowed".
			fi
			test_one_pin $base
			test_one_pin $(($base + $ngpio - 1))
			test_one_pin $((( RANDOM % $ngpio )  + $base ))
			if [ X$invalid_pin != X ]; then
				test_one_pin_fail $invalid_pin
			fi
		done
	fi
	remove_module
}

gpio_test_fail()
{
	param=$1

	insert_module "conf=$param"
	echo -n "GPIO $module fail test with base,ngpio configure<$param>: "
	gpiochip=`ls -d $GPIO_DRV_SYSFS/gpiochip* 2>/dev/null`
	if [ X"$gpiochip" = X ]; then
		echo "successful"
	else
		echo "fail"
		die
	fi
	remove_module
}

is_consistent()
{
	val=

	active_low_sysfs=`cat $GPIO_SYSFS/gpio$nr/active_low`
	val_sysfs=`cat $GPIO_SYSFS/gpio$nr/value`
	dir_sysfs=`cat $GPIO_SYSFS/gpio$nr/direction`

	gpio_this_debugfs=`cat $GPIO_DEBUGFS |grep "gpio-$nr" | sed "s/(.*)//g"`
	dir_debugfs=`echo $gpio_this_debugfs | awk '{print $2}'`
	val_debugfs=`echo $gpio_this_debugfs | awk '{print $3}'`
	if [ $val_debugfs = "lo" ]; then
		val=0
	elif [ $val_debugfs = "hi" ]; then
		val=1
	fi

	if [ $active_low_sysfs = "1" ]; then
		if [ $val = "0" ]; then
			val="1"
		else
			val="0"
		fi
	fi

	if [ $val_sysfs = $val ] && [ $dir_sysfs = $dir_debugfs ]; then
		echo -n "."
	else
		echo "test fail, exit"
		die
	fi
}

test_pin_logic()
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

test_one_pin()
{
	nr=$1

	echo -n "test pin<$nr>"

	echo $nr > $GPIO_SYSFS/export 2>/dev/null

	if [ X$? != X0 ]; then
		echo "test GPIO pin $nr failed"
		die
	fi

	#"Checking if the sysfs is consistent with debugfs: "
	is_consistent $nr

	#"Checking the logic of active_low: "
	test_pin_logic $nr out 1 1
	test_pin_logic $nr out 1 0
	test_pin_logic $nr out 0 1
	test_pin_logic $nr out 0 0

	#"Checking the logic of direction: "
	test_pin_logic $nr in 1 1
	test_pin_logic $nr out 1 0
	test_pin_logic $nr low 0 1
	test_pin_logic $nr high 0 0

	echo $nr > $GPIO_SYSFS/unexport

	echo "successful"
}

test_one_pin_fail()
{
	nr=$1

	echo $nr > $GPIO_SYSFS/export 2>/dev/null

	if [ X$? != X0 ]; then
		echo "test invalid pin $nr successful"
	else
		echo "test invalid pin $nr failed"
		echo $nr > $GPIO_SYSFS/unexport 2>/dev/null
		die
	fi
}

prerequisite

echo "1.  Do basic test: successful means insert gpiochip and manipulate gpio"
echo "    pin successful"
gpio_test
gpio_test "0,32" 33
gpio_test "0,32,32,16" 50
gpio_test "0,32,40,16,32,5" 38
echo "2.  Test dynamic allocation of gpio successful means insert gpiochip and"
echo "    manipulate gpio pin successful"
gpio_test "-1,32"
gpio_test "-1,32,32,16" 31
gpio_test "-1,32,40,16,-1,5" 38
echo "3.  Error test: successful means insert gpiochip failed"
echo "3.1 Test zero line of gpio"
gpio_test_fail "0,0"
echo "3.2 Test range overlap"
echo "3.2.1 Test corner case"
gpio_test_fail "0,32,0,1"
gpio_test_fail "0,32,32,5,32,10"
gpio_test_fail "0,32,35,5,35,10"
gpio_test_fail "0,32,31,1"
gpio_test_fail "0,32,32,5,36,1"
gpio_test_fail "0,32,35,5,34,2"
echo "3.2.2 Test inserting invalid second gpiochip"
gpio_test_fail "0,32,30,5"
gpio_test_fail "0,32,1,5"
gpio_test_fail "10,32,9,5"
gpio_test_fail "10,32,30,5"
echo "3.2.3 Test others"
gpio_test_fail "0,32,40,16,39,5"
gpio_test_fail "0,32,40,16,30,5"
gpio_test_fail "0,32,40,16,30,11"
gpio_test_fail "0,32,40,16,20,1"

echo GPIO test PASS

