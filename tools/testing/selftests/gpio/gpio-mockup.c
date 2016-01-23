
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>	//for system, random
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "gpio-mockup.h"

struct gpio_testcase_t {
	bool is_valid;
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

static int gpio_test(struct gpio_device *dev, const char *module,
		struct gpio_testcase_t *tc)
{
	int i;
	int offset = 0;
	char *insert;
	char *remove;
	int status;
	int ret;

	ret = asprintf(&insert, "/usr/sbin/modprobe -q %s gpio_mockup_ranges=\"%s\"", module, tc->ranges);
	if (ret < 0)
		return -1;

	system(insert);
	//TODO error check
	if (WIFEXITED(status))
		printf("exited, status=%d\n", WEXITSTATUS(status));

	dev->name = module;
	if (dev->init(dev))
		return -1;

	dev->list(dev);
	if (!tc->is_valid && dev->nchips > 0)
		return -1;

	if (tc->is_valid) {
		if (dev->nchips == 0)
			return -1;

		if(dev->test) {
			for (i = 0; i < dev->nchips; i++) {
				struct gpio_chip *chip = &dev->chips[i];
				dev->test(dev, chip->base);
				dev->test(dev, chip->base + chip->ngpio - 1);
				dev->test(dev, random() % chip->ngpio + chip->base);
			}
		}
	}
	dev->exit(dev);
out:
	ret = asprintf(&insert, "/usr/sbin/modprobe -q -r %s", module);
	if (ret < 0)
		return -1;

	system(remove);
	if (WIFEXITED(status))
		printf("exited, status=%d\n", WEXITSTATUS(status));

	return 0;
}

static void summary(int err)
{
	if (err > 0)
		fprintf(stderr, "GPIO test with %d error(s)\n", err);
	else
		printf ("GPIO test PASS\n");

}

int pin_get_debugfs(struct gpio_device *dev, int nr,
		struct gpio_pin_status *status)
{
	FILE *f;
	char *line = NULL;
	size_t len = 0;
	char *name;
	char *cur;

	f = fopen(dev->debugfs, "r");
	if (!f)
		return -errno;

	//append space to full word match
	asprintf(name, "gpio-%d ", nr);
	while(getline(&line, &len, f) != -1) {
		// gpio-2   (                    |sysfs               ) in  lo
		if (!strstr(line, name))
			continue;

		cur = strchr(line, ')');
		if (!cur)
			continue;

		cur += 2;
		if (!strncmp(cur, "out", 3)) {
			status->dir = OUT;
			cur += 4;
		} else if (!strncmp(cur, "in", 2)) {
			status->dir = IN;
			cur += 3;
		}

		if (!strncmp(cur, "hi", 2))
			status->value = true;
		else if (!strncmp(cur, "lo", 2))
			status->value = false;

		break;
	}
	fclose(f);
	free(line);
	free(name);
	return 0;
}

int main(int argc, char *argv[])
{
	struct gpio_device *dev;
	char *module;
	int err = 0;
	bool is_char = true;
	int opt;
	int i;

	while ((opt = getopt(argc, argv, "m:i:")) != -1) {
		switch (opt) {
		case 'm':
			module = strdup(optarg);
			break;
		case 'i':
			if (!strcmp(optarg, "sysfs"))
				is_char = false;

			break;
		default: /* '?' */
			fprintf(stderr, "Usage: %s -m module -i chardev/sysfs\n",
			   argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "Expected argument after options\n");
		exit(EXIT_FAILURE);
	}

	if (is_char)
//		dev = gpio_chardev;
		exit(EXIT_FAILURE);
	else
		dev = &gpio_sysfs;

	prerequisite();
	for (i = 0; i< sizeof(gpio_testcases)/sizeof(struct gpio_testcase_t);
	     i++ ) {
		if (gpio_test(dev, module, &gpio_testcases[i]) < 0)
			err++;
	}
	summary(err);
	free(module);

	return err > 0 ? -1 : 0;
}

