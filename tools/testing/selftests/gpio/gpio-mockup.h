
#ifndef GPIO_MOCKUP_H
#define GPIO_MOCKUP_H

#include <stdbool.h>

struct gpio_device gpio_sysfs;
struct gpio_device gpio_chardev;

struct gpio_chip {
	char *name;
};

enum direction
{
	INVAL = -1,
	IN = 0,
	OUT,
	HIGH,
	LOW,
};

struct gpio_pin_status {
	enum direction	dir;
	bool		value;
	bool		active_low;
};

struct gpio_device {
	char *name;
	struct gpio_chip *chips;
	int nchips;
	char *debugfs;
	int (*init)(struct gpio_device *dev);
	void (*exit)(struct gpio_device *dev);
	int (*list)(struct gpio_device *dev);
	int (*test)(struct gpio_device *dev, unsigned pin);
	void *private;
};

int pin_get_debugfs(struct gpio_device *dev, int nr,
		struct gpio_pin_status *status);
#endif /* #ifndef GPIO_MOCKUP_H */
