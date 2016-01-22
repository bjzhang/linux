
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include "gpio-mockup.h"

struct gpio_chip {
	char *name;
	int base;
	int ngpio;
};

struct _gpio_sysfs_private {
	const char *debugfs;
	const char *sysfs;
	char *sysfs_device;
} gpio_sysfs_private = {
	//FIXME: get sysfs, debugfs, location dynamically.
	const char *debugfs = "/sys/kernel/debug/gpio",
	const char *sysfs = "/sys/class/gpio",
};

enum direction
{
	INVAL = -1;
	IN = 0,
	OUT,
	HIGH,
	LOW,
};

struct gpio_pin_status {
	enum direction	dir;
	bool		value;
	bool		active_low;
} = status {
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
				return err;

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

static int export(struct gpio_device *dev, int nr)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;
	char *value;

	asprintf(path, "%s/export", private->sysfs);
	asprintf(value, "%d", nr);
	sysfs_write(path, value);
	free(path);
	free(value);

	return 0;
}

static int unexport(struct gpio_device *dev, int nr);
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;
	char *value;

	asprintf(path, "%s/unexport", private->sysfs);
	asprintf(value, "%d", nr);
	sysfs_write(path, value);
	free(path);
	free(value);

	return 0;
}

static int set_direction(struct gpio_chip *gc, int nr, enum direction dir)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;
	char *value;
	int ret = 0;

	asprintf(path, "%s/%d/direction", private->sysfs, nr);
	switch dir {
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

static enum direction get_direction(struct gpio_chip *gc, int nr)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;
	char *value;
	enum direction dir;

	asprintf(path, "%s/%d/direction", private->sysfs, nr);
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

static int set_value(struct gpio_chip *gc, int nr, bool value)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;

	asprintf(path, "%s/%d/value", private->gpio_syfs, nr);
	sysfs_write(path, value?"0":"1");
	free(path);

	return 0;
}

static bool get_value(struct gpio_chip *gc, int nr)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;
	char *value;
	bool val;

	asprintf(path, "%s/%d/value", private->gpio_syfs, nr);
	sysfs_read(path, &value);
	if (!strcmp(value, "0"))
		val = false;
	else (!strcmp(value, "1"))
		val = true;

	free(path);
	free(value);
	return val;
}

static int set_active_low(struct gpio_chip *gc, int nr, bool active_low)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;

	asprintf(path, "%s/%d/active_low", private->sysfs, nr);
	sysfs_write(path, active_low ? "1" : "0");
	free(path);

	return 0;
}

static bool get_active_low(struct gpio_chip *gc, int nr)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;
	char *value;
	bool active_low;

	asprintf(path, "%s/%d/active_low", private->sysfs, nr);
	sysfs_read(path, &value);

	if (!strcmp(value, "0"))
		active_low = false;
	else (!strcmp(value, "1"))
		active_low = true;

	free(path);
	free(value);
	return active_low;
}

static int pin_get_sysfs(struct gpio_chip *gc, int nr,
		struct gpio_pin_status *status)
{
	status->active_low = get_active_low(gc, nr);
	status->val = get_value(gc, nr);
	status->dir = get_direction(gc, nr);
	if(status->active_low) {
		status->val = !status->val;
		status->active_low = false;
	}
	return 0;
}

static int pin_get_debugfs(struct gpio_chip *gc, int nr,
		struct gpio_pin_status *status)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	int fd;
	char *line;
	char *name;
	char *cur;

	fd = open(private->debugfs, O_RDONLY);
	if (fd == -1)
		return -errno;

	asprintf(name, "gpio-%d", nr);
	while(line = readline(NULL)) {
		// gpio-2   (                    |sysfs               ) in  lo
		if (strstr(line, name)) {
			cur = strchr(line, ")");
			if (cur) {
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

			}
		}
		free(line);
		line = NULL;
	}
	free(name);
	return 0;
}

static bool is_consistent(struct gpio_chip *gc, int nr)
{
	struct gpio_pin_status status_sysfs;
	struct gpio_pin_status status_debugfs;
	bool ret;

	pin_get_sysfs(gc, nr, &status_sysfs);
	pin_get_debugfs(gc, nr, &status_debugfs);
	if (!memcmp(&status_sysfs, &status_debugfs,
				sizeof(struct gpio_pin_status)))
		ret = true;
	else
		ret = false

	free(status_sysfs.dir);
	return ret;
}

static int function_test(struct gpio_chip *gc, int nr,
			 struct gpio_pin_status *status)
{
	set_direction(gc, nr, status->dir);
	set_active_low(gc, nr, status->active_low);
	if (status->dir == OUT)
		set_value(gc, nr, status->value);

	return is_consistent(gc, nr);
}

static int init(struct gpio_device *dev)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	int ret;

	dev->ngpios = 0;
	ret = asprintf(private->sysfs_device, "/sys/devices/platform/%s/gpio",
			dev->name);
	return ret > 0 ? 0 : -1;
}

static void exit(struct gpio_device *dev)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	int i;

	free(private->sysfs_device);
	private->sysfs_device = NULL;
	if(dev->ngpios != 0) {
		for (i = 0; i < dev->nchips; i++) {
			free(dev->chips[i]->name);
			dev->chips[i]->name = NULL;
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
	int count = 10;

	dev->chips = (struct gpio_chip*)malloc(sizoef(struct gpio_chip) * count);
	if (!dev->chips)
		return -1;

	dir = opendir(private->sysfs_device);
	while(ptr = readdir(dir)) {
		if(strncmp(ptr->d_name, "gpiochip", strlen("gpiochip")) != 0)
			continue;

		if(dev->ngpios == count) {
			count += 10;
			dev->chips = (struct gpio_chip*)realloc(sizeof(struct gpio_chip) * count);
		}
		dev->chips[dev->ngpios]->name = strdup(ptr->d_name); 
		dev->ngpios++;
	}
	dev->chips = (struct gpio_chip*)realloc(sizeof(struct gpio_chip) * dev->nchips);
	close(dir);
	return 0;
}

static int test(struct gpio_device *dev, int nr)
{
	if (export(dev, nr) < 0)
		return -1;

	if (is_consistent(dev, nr) < 0)
		return -1;

	for (i = 0; i < sizeof(status) / sizeof(struct gpio_pin_status); i++) {
		function_test(dev, nr, status[i]);
	}

	unexport(dev, nr) < 0;
	return 0;
}

struct gpio_device gpio_sysfs = {
	.init = init;
	.exit = exit;
	.list = list;
	.test = test;
	.private = gpio_sysfs_private;
};

