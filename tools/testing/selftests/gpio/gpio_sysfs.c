
#include <errno.h>

struct gpio_chip {
	char *name;
	int base;
	int ngpio;
};

struct _gpio_sysfs_private {
	const char *gpio_debugfs;
	const char *gpio_sysfs;
	char *gpio_drv_sysfs;
} gpio_sysfs_private = {
	//FIXME: get sysfs, debugfs, location dynamically.
	const char *gpio_debugfs = "/sys/kernel/debug/gpio",
	const char *gpio_sysfs = "/sys/class/gpio",
};

struct gpio_device gpio_sysfs = {
	.init = init;
	.exit = exit;
	.list = list;
	.test = test;
	.private = gpio_sysfs_private;
};

enum direction {
	INVAL	= -1
	IN	= 0,
	OUT
};

struct gpio_pin_status {
	const char	*dir;
	bool		value;
	bool		active_low;
} = status {
	{"out", true, true},
	{"out", true, false},
	{"out", false, true},
	{"out", false, true},
	{"in", true, true},
	{"out", true, false},
	{"low", true, true},
	{"high", true, true}
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
	int fd;

	asprintf(path, "%s/export", private->gpio_sysfs);
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

	asprintf(path, "%s/unexport", private->gpio_sysfs);
	asprintf(value, "%d", nr);
	sysfs_write(path, value);
	free(path);
	free(value);

	return 0;
}

static int set_direction(struct gpio_chip *gc, int nr, const char *dir)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;
	char *value;
	int ret = 0;

	asprintf(path, "%s/%d/direction", private->gpio_sysfs, nr);
	if(sysfs_write(path, dir) < 0)
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
	enum direction dir = OUT;

	asprintf(path, "%s/%d/direction", private->gpio_sysfs, nr);
	if (sysfs_read(path, value) < 0)
		dir = INVAL;

	if (!strcmp(dir, "in"))
		dir = IN;
	else if (!strcmp(dir, "out"))
		dir = OUT;

	free(path);

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
	bool value;

	asprintf(path, "%s/%d/value", private->gpio_syfs, nr);
	sysfs_read(path, value);
	if (!strcmp(value, "0"))
		value = false;
	else if (!strcmp(value, "1"))
		value = true;

	free(path);

	return 0;
}
static int set_active_low(struct gpio_chip *gc, int nr, bool active_low)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;

	asprintf(path, "%s/%d/active_low", private->gpio_sysfs, nr);
	sysfs_write(path, active_low?"1":"0");
	free(path);

	return 0;
}


static int is_consistent(struct gpio_chip *gc, int nr)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;
	char *active_low_sysfs;
	char *val_sysfs;
	char *dir_sysfs;
	int dir_debugfs;
	int val_debugfs;

	asprintf(path, "%s/%d/active_low", private->gpio_sysfs, nr);
	sysfs_read(path, &active_low_sysfs);
	asprintf(path, "%s/%d/value", private->gpio_sysfs, nr);
	sysfs_read(path, &value);
	asprintf(path, "%s/%d/dir", private->gpio_sysfs, nr);
	sysfs_read(path, &dir);


}

static int function_test(struct gpio_chip *gc, int nr,
			 struct gpio_pin_status *status)
{
	set_direction(gc, nr, status->dir);
	set_active_low(gc, nr, status->active_low);
	if (!!strcmp(status->dir, "in"))
		set_value(gc, nr, status->value);

	return is_consistent(gc, nr);
}

static int init(struct gpio_device *dev)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	int ret;

	ret = asprintf(private->gpio_drv_sysfs, "/sys/devices/platform/%s/gpio",
			dev->name);
	return ret > 0 ? 0 : ret;
}

static void exit(struct gpio_device *dev)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	free(private->gpio_drv_sysfs);
	private->gpio_drv_sysfs = NULL;
}

static int test(struct gpio_device *dev, int nr, bool is_valid)
{
	if (export(dev, nr) < 0)
		return -1;

	if (is_consistent(dev, nr) < 0)
		if (is_valid)
			return -1;
		else
			return 0;

	for (i = 0; i < sizeof(status) / sizeof(struct gpio_pin_status); i++) {
		function_test(dev, nr, status[i]);
	}

	unexport(dev, nr) < 0;
	return 0;
}

