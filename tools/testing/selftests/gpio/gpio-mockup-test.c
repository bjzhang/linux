
#include <stdio.h>
#include <stdbool.h>

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

//TODO: get sysfs, debugfs, location
const char *gpio_sysfs = "/sys/class/gpio"
const char *gpio_drv_sysfs = "/sys/devices/platform/gpio-mockup/gpio`
const char *gpio_debugfs = "/sys/kernel/debug/gpio"

void prerequisite()
{
	//TODO: must be run as root
	printf("uid<%d>, euid<%d>\n", getuid(), geteuid());
}

int gpio_test(struct gpio_testcase_t *tc)
{
	int i;
	int offset = 0;
	char *insert_module = "/usr/sbin/modprobe -q gpio_mockup_bases=\"xxx,xxx,xxx,xxx,xxx\" gpio_mockup_ngpios=\"xxx,xxx,xxx,xxx,xxx\"";
	char *bases = "xxx,xxx,xxx,xxx,xxx";
	char *ngpios = "xxx,xxx,xxx,xxx,xxx";

	offset = 0;
	for (i = 0; i < tc->nr_gc; i++){
		if (i != tc->nr_gc - 1 )
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
		snprintf(insert_module, strlen(insert_module), "/usr/sbin/modprobe -q");
	else
		snprintf(insert_module, strlen(insert_module), "/usr/sbin/modprobe -q gpio_mockup_bases=\"%s\"gpio_mockup_ngpios=\"%s\"", bases, ngpios);

	system(insert_module);
	//TODO error check

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


}

int main(int argc, char *argv[])
{
	struct gpio_testcase_t *t;
	int i, j;

	prerequisite();
	for( i = 0; i < sizeof(gpio_testcases) / sizeof(struct gpio_testcase_t); i++) {
		printf("%d\n", i);
		t = &gpio_testcases[i];
		if (t->nr_bases != 0) {
			for ( j = 0; j < t->nr_bases; j++) {
				printf("bases: %d\n", t->bases[j]);
			}
		}
		if (t->nr_ngpios != 0) {
			for ( j = 0; j < t->nr_ngpios; j++) {
				printf("ngpios: %d\n", t->ngpios[j]);
			}
		}
		if (t->invalid_pin != -1) {
			printf("invalid_pin: %d\n", t->invalid_pin);
		}
		printf("expect_successful: %s\n",
		       t->expect_successful ? "true" : "false");
	}
	printf ("GPIO test PASS\n");
}

