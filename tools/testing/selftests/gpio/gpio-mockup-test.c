
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>	//for system
#include "gpio-mockup.h"

#define MAX_GC 5
struct gpio_testcase_t {
	bool is_valid_gc;
	char ranges[];
} gpio_testcases[] = {
	//1.  Do basic test: successful means insert gpiochip and manipulate gpio pin successful
	{true, "0, 32"},
	{true, "0,32,32,64"},
	{true, "0,32,40,56,64,70"},
	//2.  Test dynamic allocation of gpio successful means insert gpiochip and manipulate gpio pin successful
	{true, "-1,32"},
	{true, "-1, 32, 0, 32"},
	{true, "-1, 32, 32, 64"},
	//{{-1, 32}, {32,16}, 2, 31, true},
	//{{-1, 40, -1}, {32, 16, 5}, 3, 38, true},
	//3.  Error test: successful means insert gpiochip failed
	//3.1 Test zero line of gpio
//	gpio_test_fail "0,0"
//	printf("3.2 Test range overlap\n")
//	printf("3.2.1 Test corner case\n")
//	gpio_test_fail "0,32,0,1"
//	gpio_test_fail "0,32,32,5,32,10"
//	gpio_test_fail "0,32,35,5,35,10"
//	gpio_test_fail "0,32,31,1"
//	gpio_test_fail "0,32,32,5,36,1"
//	gpio_test_fail "0,32,35,5,34,2"
//	printf("3.2.2 Test inserting invalid second gpiochip\n")
//	gpio_test_fail "0,32,30,5"
//	gpio_test_fail "0,32,1,5"
//	gpio_test_fail "10,32,9,5"
//	gpio_test_fail "10,32,30,5"
//	printf("3.2.3 Test others\n")
//	gpio_test_fail "0,32,40,16,39,5"
//	gpio_test_fail "0,32,40,16,30,5"
//	gpio_test_fail "0,32,40,16,30,11"
//	gpio_test_fail "0,32,40,16,20,1"
};

static void prerequisite()
{
	//TODO: must be run as root
	printf("uid<%d>, euid<%d>\n", getuid(), geteuid());
}

static int gpio_test(const char *module, struct gpio_testcase_t *tc)
{
	int i;
	int offset = 0;
	char *insert;
	char *remove;
	int ret;
	struct gpio_device *dev;

	ret = asprintf(&insert, "/usr/sbin/modprobe -q %s gpio_mockup_ranges=\"%s\"", module, tc->ranges);
	if (ret < 0)
		return -1;

	//TODO: check whether range conflict with existing gpio drivers
	system(insert);
	//TODO error check

	//TODO default char dev
	if (0)
		dev = gpio_chardev;
	else
		dev = gpio_sysfs;

	dev->name = module;
	if (dev->init(dev))
		return -1;

	dev->list(dev);
	if (!tc->is_valid_gc && dev->nchips > 0)
		return -1;

	if (tc->is_valid_gc) {
		if (dev->nchips == 0)
			return -1;

		if(dev->test) {
			for (i = 0; i < dev->nchips; i++) {
				struct gpio_chip *chip = dev->chips[i];
				dev->test(dev, chip->base, true);
				dev->test(dev, chip->base + chip->ngpio - 1, true);
				dev->test(dev, random(chip->base, chip->base + chip->ngpio - 1), true);
			}
		}
	}
	dev->exit(dev);
out:
	ret = asprintf(&insert, "/usr/sbin/modprobe -q -r %s", module);
	if (ret < 0)
		return -1;

	system(remove);
}

static void summary(int err)
{
	if (err > 0)
		fprintf(stderr, "GPIO test with %d error\n", err);
	else
		printf ("GPIO test PASS\n");
}

int main(int argc, char *argv[])
{
	char *module;
	int err = 0;
	int i;

	//TODO: paramter:
	//-m module_name
	//-t "force one test case"
	//-s "force use sysfs interface no matter char device exist or not".
	prerequisite();
	for (i = 0; i< sizeof(gpio_testcases)/sizeof(struct gpio_testcase_t);
	     i++ ) {
		if (gpio_test(module, gpio_testcases[i]) < 0)
			err++;
	}
	summary(err);
}

