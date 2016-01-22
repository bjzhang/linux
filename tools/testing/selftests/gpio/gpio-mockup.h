
#ifndef GPIO_MOCKUP_H
#define GPIO_MOCKUP_H

struct gpio_chip;
struct gpio_device gpio_sysfs;
struct gpio_device gpio_chardev;

struct gpio_device {
	char *name;
	struct gpio_chip *chips;
	int nchips;
	int (*init)(struct gpio_device *dev);
	void (*exit)(struct gpio_device *dev);
	int (*list)(struct gpio_device *dev);
	int (*test)(struct gpio_device *dev, unsigned pin);
	void *private;
};

#endif /* #ifndef GPIO_MOCKUP_H */
