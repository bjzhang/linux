
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include "gpio-mockup.h"

struct _gpio_sysfs_private {
	const char *class;
	char *device;
} gpio_sysfs_private = {
	//FIXME: get sysfs, debugfs, location dynamically.
	.class = "/sys/class/gpio",
};

struct gpio_pin_status status[] = {
	{OUT, true, true},
	{OUT, true, false},
	{OUT, false, true},
	{OUT, false, false},
	{IN, true, true},
	{OUT, true, false},
	{LOW, true, true},
	{HIGH, true, true}
};

static int sysfs_read(const char *path, char **valuep)
{
	int fd;
	int count = 10;
	int total = 0;
	int actual;

	fd = open(path, O_RDONLY);
	if (fd == -1)
		return -errno;

	*valuep = NULL;
	*valuep = malloc(count * sizeof(char));
	if (!*valuep)
		goto err;

	while(actual != 0) {
retry:
		actual = read(fd, (*valuep + total), count);
		switch(actual) {
		case -1:
			if (errno == EAGAIN || errno == EINTR)
				goto retry;
			else
				goto err;

			break;
		case 0:
			*valuep = realloc(*valuep, total);
			if (*valuep == NULL)
				goto err;

			break;
		default:
			total += actual;
			*valuep = realloc(*valuep, total + count);
			if (*valuep == NULL)
				goto err;

			break;
		}
	}

	return 0;
err:
	close(fd);
	free(*valuep);
	*valuep = NULL;
	return errno;
}

static int sysfs_write(const char *path, const char *value)
{
	int fd;
	int ret;

	fd = open(path, O_WRONLY);
	if (fd == -1)
		return -errno;

	ret = write(fd, value, strlen(value));
	if (ret < 0) {
		close(fd);
		return -errno;
	}

	ret = close(fd);
	if (ret < 0)
		return -errno;
	else
		return 0;
}

static int export(struct gpio_device *dev, unsigned pin)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;
	char *value;

	asprintf(path, "%s/export", private->class);
	asprintf(value, "%d", pin);
	sysfs_write(path, value);
	free(path);
	free(value);

	return 0;
}

static int unexport(struct gpio_device *dev, unsigned pin)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;
	char *value;

	asprintf(path, "%s/unexport", private->class);
	asprintf(value, "%d", pin);
	sysfs_write(path, value);
	free(path);
	free(value);

	return 0;
}

static int set_direction(struct gpio_device *dev, unsigned pin, enum direction dir)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;
	char *value;
	int ret = 0;

	asprintf(path, "%s/%d/direction", private->class, pin);
	switch (dir) {
		case IN:
			value = "in";
			break;
		case OUT:
			value = "out";
			break;
		case LOW:
			value = "low";
			break;
		case HIGH:
			value = "high";
			break;
	}
	if(sysfs_write(path, value) < 0)
		ret = -1;

	free(path);
	return ret;
}

static enum direction get_direction(struct gpio_device *dev, unsigned pin)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;
	char *value;
	enum direction dir;

	asprintf(path, "%s/%d/direction", private->class, pin);
	if (sysfs_read(path, &value) < 0) {
		dir = INVAL;
		goto out;
	}
	if (!strcmp(value, "out"))
		dir = OUT;
	else if (!strcmp(value, "int"))
		dir = IN;

out:
	free(path);
	free(value);
	return dir;
}

static int set_value(struct gpio_device *dev, unsigned pin, bool value)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;

	asprintf(path, "%s/%d/value", private->class, pin);
	sysfs_write(path, value?"0":"1");
	free(path);

	return 0;
}

static bool get_value(struct gpio_device *dev, unsigned pin)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;
	char *value;
	bool val;

	asprintf(path, "%s/%d/value", private->class, pin);
	sysfs_read(path, &value);
	if (!strcmp(value, "0"))
		val = false;
	else if (!strcmp(value, "1"))
		val = true;

	free(path);
	free(value);
	return val;
}

static int set_active_low(struct gpio_device *dev, unsigned pin, bool active_low)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;

	asprintf(path, "%s/%d/active_low", private->class, pin);
	sysfs_write(path, active_low ? "1" : "0");
	free(path);

	return 0;
}

static bool get_active_low(struct gpio_device *dev, unsigned pin)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;
	char *value;
	bool active_low;

	asprintf(path, "%s/%d/active_low", private->class, pin);
	sysfs_read(path, &value);

	if (!strcmp(value, "0"))
		active_low = false;
	else if (!strcmp(value, "1"))
		active_low = true;

	free(path);
	free(value);
	return active_low;
}

static int pin_get_sysfs(struct gpio_device *dev, unsigned pin,
		struct gpio_pin_status *status)
{
	status->active_low = get_active_low(dev, pin);
	status->value = get_value(dev, pin);
	status->dir = get_direction(dev, pin);
	if(status->active_low) {
		status->value = !status->value;
		status->active_low = false;
	}
	return 0;
}

static bool is_consistent(struct gpio_device *dev, unsigned pin)
{
	struct gpio_pin_status status_sysfs;
	struct gpio_pin_status status_debugfs;
	bool ret;

	pin_get_sysfs(dev, pin, &status_sysfs);
	pin_get_debugfs(dev, pin, &status_debugfs);
	if (!memcmp(&status_sysfs, &status_debugfs,
				sizeof(struct gpio_pin_status)))
		ret = true;
	else
		ret = false;

	return ret;
}

static int function_test(struct gpio_device *dev, unsigned pin,
			 struct gpio_pin_status *status)
{
	set_direction(dev, pin, status->dir);
	set_active_low(dev, pin, status->active_low);
	if (status->dir == OUT)
		set_value(dev, pin, status->value);

	return is_consistent(dev, pin);
}

static int device_init(struct gpio_device *dev)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	int ret;

	dev->nchips = 0;
	ret = asprintf(private->device, "/sys/devices/platform/%s/gpio",
			dev->name);
	return ret > 0 ? 0 : -1;
}

static void device_exit(struct gpio_device *dev)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	int i;

	free(private->device);
	private->device = NULL;
	if(dev->nchips != 0) {
		for (i = 0; i < dev->nchips; i++) {
			free(dev->chips[i].name);
			dev->chips[i].name = NULL;
		}
		free(dev->chips);
		dev->chips = NULL;
	}
}

static int list(struct gpio_device *dev)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	DIR *dir;
	struct dirent *ptr;
	struct gpio_chip *chip;
	int count = 10;
	char *path;
	char *value;

	dev->chips = (struct gpio_chip*)malloc(sizeof(struct gpio_chip) * count);
	if (!dev->chips)
		return -1;

	dir = opendir(private->device);
	while(ptr = readdir(dir)) {
		if(strncmp(ptr->d_name, "gpiochip", strlen("gpiochip")) != 0)
			continue;

		if(dev->nchips == count) {
			count += 10;
			dev->chips = (struct gpio_chip*)realloc(dev->chips,
					sizeof(struct gpio_chip) * count);
		}
		chip = &dev->chips[dev->nchips];
		chip->name = strdup(ptr->d_name);
		asprintf(path, "%s/%s/base", private->device, chip->name);
		sysfs_read(path, &value);
		chip->base = atoi(value);
		free(path);
		free(value);
		asprintf(path, "%s/%s/ngpio", private->device, chip->name);
		sysfs_read(path, &value);
		chip->ngpio = atoi(value);
		free(path);
		free(value);
		dev->nchips++;
	}
	dev->chips = (struct gpio_chip*)realloc(dev->chips,
			sizeof(struct gpio_chip) * dev->nchips);
	closedir(dir);
	return 0;
}

static int test(struct gpio_device *dev, unsigned pin)
{
	int i;

	if (export(dev, pin) < 0)
		return -1;

	if (is_consistent(dev, pin) < 0)
		return -1;

	for (i = 0; i < sizeof(status) / sizeof(struct gpio_pin_status); i++) {
		function_test(dev, pin, &status[i]);
	}

	unexport(dev, pin) < 0;
	return 0;
}

struct gpio_device gpio_sysfs = {
	//FIXME: get class, debugfs, location dynamically.
	.debugfs = "/sys/kernel/debug/gpio",
	.init = device_init,
	.exit = device_exit,
	.list = list,
	.test = test,
	.private = &gpio_sysfs_private
};

