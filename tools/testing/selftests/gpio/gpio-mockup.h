
struct gpio_chip;

struct gpio_device {
	char *name;
	struct gpio_chip *chips;
	int nchips;
	int (*init)(struct gpio_device *dev);
	void (*exit)(struct gpio_device *dev);
	int (*list)(struct gpio_device *dev);
	int (*test)(struct gpio_device *dev, unsigned pin, bool is_valid);
	void *private;
};

