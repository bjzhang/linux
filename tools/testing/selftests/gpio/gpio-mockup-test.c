
#include <stdio.h>
#include <stdbool.h>

#define MAX_GC 5
struct gpio_testcase_t {
	int bases[MAX_GC];
	int nr_bases;
	int ngpios[MAX_GC];
	int nr_ngpios;
	int invalid_pin;
	bool expect_successful;
} gpio_testcases[] = {
	//1.  Do basic test: successful means insert gpiochip and manipulate gpio pin successful
	{{0}, 0, {0}, 0, -1, true},
	{{0}, 1, {32}, 1, 33, true},
	{{0,32}, 2, {32,16}, 2, 50, true},
	{{0, 40, 32}, 3, {32, 16, 5}, 3, 38, true},
	//2.  Test dynamic allocation of gpio successful means insert gpiochip and manipulate gpio pin successful
	{{-1}, 1, {32}, 1, 0, true},
	{{-1, 32}, 2, {32,16}, 2, 31, true},
	{{-1, 40, -1}, 3, {32, 16, 5}, 3, 38, true},
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

void prerequisite();
int gpio_test(int *bases, int *ngpios, int *invalid_pins, bool expect_ret);

int main(int argc, char *argv[])
{
	struct gpio_testcase_t *t;
	int i, j;

//	prerequisite();
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

