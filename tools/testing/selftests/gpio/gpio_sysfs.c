
struct gpio_chip {
	char *name;
	int base;
	int ngpio;
};

//TODO: get sysfs, debugfs, location

struct _gpio_sysfs_private {
	const char *gpio_debugfs;
	const char *gpio_sysfs;
	char *gpio_drv_sysfs;
} gpio_sysfs_private = {
	const char *gpio_debugfs = "/sys/kernel/debug/gpio",
	const char *gpio_sysfs = "/sys/class/gpio",
}

struct gpio_device gpio_sysfs = {
	.init = init;
	.exit = exit;
	.list = list;
	.test = test;
	.private = gpio_sysfs_private;
};

enum direction {
	IN,
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

static int export(struct gpio_device *dev, int nr)
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;
	char *number;
	int fd;

	asprintf(path, "%s/export", private->gpio_sysfs);
	asprintf(number, "%d", nr);
	//TODO error check
	fd = open(path, O_WRONLY);
	//TODO error check
	write(fd, number);
	close(fd);

	free(path);
	free(number);
	return 0;
}

static int unexport(struct gpio_device *dev, int nr);
{
	struct _gpio_sysfs_private *private =
		(struct _gpio_sysfs_private*)dev->private;
	char *path;
	char *number;
	int fd;

	asprintf(path, "%s/unexport", private->gpio_sysfs);
	asprintf(number, "%d", nr);
	//TODO error check
	fd = open(path, O_WRONLY);
	//TODO error check
	write(fd, number);
	close(fd);

	free(path);
	free(number);
	return 0;
}

static int is_consistent(struct gpio_chip *gc, int nr);
static int set_direction(struct gpio_chip *gc, int nr, const chat *dir);
static int set_value(struct gpio_chip *gc, int nr, bool value);
static int set_active_low(struct gpio_chip *gc, int nr, bool active_low);

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

