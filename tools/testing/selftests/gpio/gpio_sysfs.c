
struct gpio_chip {

	void *private;
};

struct gpio_sysfs_private {
	const char *sysfs;
}

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

static int export(struct gpio_chip *gc, int nr)
{
	struct gpio_sysfs_private *private =
		(struct gpio_sysfs_private*)gc->private;
	char *path;
	int fd;

	path = asprintf("%s/export", private->sysfs);
	//TODO error check
	fd = open(path, O_WRONLY);
	//TODO error check
	write(fd, nr);
	close(fd);

	free(path);
	return 0;
}

static int unexport(struct gpio_chip *gc, int nr);
{
	struct gpio_sysfs_private *private =
		(struct gpio_sysfs_private*)gc->private;
	char *path;
	int fd;

	path = asprintf("%s/unexport", private->sysfs);
	//TODO error check
	fd = open(path, O_WRONLY);
	//TODO error check
	write(fd, nr);
	close(fd);

	free(path);
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

int pin_test(struct gpio_chip *gc, int nr)
{
	if (export(gc, nr) < 0)
		return -1;

	if (is_consistent() < 0)
		return -1;

	for (i = 0; i < sizeof(status) / sizeof(struct gpio_pin_status); i++) {
		function_test(gc, nr, status[i]);
	}

	unexport(gc, nr) < 0;
	return 0;
}

