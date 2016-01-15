
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>	//for system

#define MAX_GC 5
struct gpio_testcase_t {
	int bases[MAX_GC];
	int ngpios[MAX_GC];
	int nr_gc;
	int invalid_pin;
	bool is_valid_gc;
} gpio_testcases[] = {
	//1.  Do basic test: successful means insert gpiochip and manipulate gpio pin successful
	{{0}, {0}, 0, -1, true},
	{{0}, {32}, 1, 33, true},
	{{0,32}, {32,16}, 2, 50, true},
	{{0, 40, 32}, {32, 16, 5}, 3, 38, true},
	//2.  Test dynamic allocation of gpio successful means insert gpiochip and manipulate gpio pin successful
	{{-1}, {32}, 1, 0, true},
	{{-1, 32}, {32,16}, 2, 31, true},
	{{-1, 40, -1}, {32, 16, 5}, 3, 38, true},
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

void prerequisite()
{
	//TODO: must be run as root
	printf("uid<%d>, euid<%d>\n", getuid(), geteuid());
}

int test(const char *module, struct gpio_testcase_t *tc)
{
	int i;
	int offset = 0;
	char *insert;
	char *remove;
	char *bases = "xxx,xxx,xxx,xxx,xxx";	//The max gpio is 512.
	char *ngpios = "xxx,xxx,xxx,xxx,xxx";
	int ret;
	struct gpio_device *dev;

	offset = 0;
	for (i = 0; i < tc->nr_gc; i++){
		if (i != tc->nr_gc - 1 )
			asprintf
			offset = snprintf(bases + offset, sizeof(bases), "%d,", tc->bases[i]);
		else
			offset = snprintf(bases + offset, sizeof(bases), "%d", tc->bases[i]);

		//TODO:
		if (offset <= 0)
			return -1;
	}

	offset = 0;
	for (i = 0; i < tc->nr_gc; i++){
		if (i != tc->nr_gc - 1 )
			offset = snprintf(ngpios + offset, sizeof(ngpios), "%d,", tc->ngpios[i]);
		else
			offset = snprintf(ngpios + offset, sizeof(ngpios), "%d", tc->ngpios[i]);

		//TODO:
		if (offset <= 0)
			return -1;
	}

	if (tc->gc_nr == 0)
		ret = asprintf(&insert, "/usr/sbin/modprobe -q %s", module);
	else
		ret = asprintf(&insert, "/usr/sbin/modprobe -q %s gpio_mockup_bases=\"%s\"gpio_mockup_ngpios=\"%s\"", module, bases, ngpios);

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

	if (tc->is_valid_gc) {
		dev->name = module;
		if (dev->init(dev))
			return -1;

		dev->list(dev);
		for (i = 0; i < dev->nchips; i++) {
			struct gpio_chip *chip = dev->chips[i];
			dev->test(dev, chip->base, true);
			dev->test(dev, chip->base + chip->ngpio - 1, true);
			dev->test(dev, random(chip->base, chip->base + chip->ngpio - 1), true);
		}
		dev->test(dev, tc->invalid_pin, false);
	}
	system(remove_module);
}

void summary()
{
	printf ("GPIO test PASS\n");
}

int main(int argc, char *argv[])
{
	struct gpio_testcase_t *t;
	int i, j;

	//TODO: paramter:
	//-m module_name
	//-t "force one test case"
	//-s "force use sysfs interface no matter char device exist or not".
	prerequisite();
	test();
	summary();
}

