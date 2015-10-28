/*
 * GPIO Testing Device Driver
 *
 * Copyright (C) 2014  Kamlakant Patel <kamlakant.patel@linaro.org>
 * Copyright (C) 2015  Bamvor Jian Zhang <bamvor.zhangjian@linaro.org>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio/driver.h>
#include <linux/platform_device.h>

#define GPIO_NAME	"gpio-mockup"
#define	MAX_GC		5

enum direction {
	IN,
	OUT
};

/*
 * struct gpio_pin_status - structure describing a GPIO status
 * @dir:       Configures direction of gpio as "in" or "out", 0=in, 1=out
 * @value:     Configures status of the gpio as 0(low) or 1(high)
 */
struct gpio_pin_status {
	enum direction	dir;
	int		value;
};

struct mockup_gpio_controller {
	struct gpio_chip	gc;
	struct gpio_pin_status	*stats;
};

int conf[MAX_GC << 1];
int params_nr;
module_param_array(conf, int, &params_nr, 0400);

const char *pins_name[MAX_GC] = {"A", "B", "C", "D", "E"};

static int
mockup_gpio_get(struct gpio_chip *gc, unsigned offset)
{
	struct mockup_gpio_controller *cntr = container_of(gc,
			struct mockup_gpio_controller, gc);

	return cntr->stats[offset].value;
}

static void
mockup_gpio_set(struct gpio_chip *gc, unsigned offset, int value)
{
	struct mockup_gpio_controller *cntr = container_of(gc,
			struct mockup_gpio_controller, gc);

	if (value)
		cntr->stats[offset].value = 1;
	else
		cntr->stats[offset].value = 0;

}

static int mockup_gpio_dirout(struct gpio_chip *gc, unsigned offset,
		int value)
{
	struct mockup_gpio_controller *cntr = container_of(gc,
			struct mockup_gpio_controller, gc);

	mockup_gpio_set(gc, offset, value);
	cntr->stats[offset].dir = OUT;
	return 0;
}

static int mockup_gpio_dirin(struct gpio_chip *gc, unsigned offset)
{
	struct mockup_gpio_controller *cntr = container_of(gc,
			struct mockup_gpio_controller, gc);

	cntr->stats[offset].dir = IN;
	return 0;
}

int mockup_gpio_add(struct device *dev, struct mockup_gpio_controller *cntr,
		const char *name, int base, int ngpio)
{
	int ret;

	cntr->gc.base = base;
	cntr->gc.ngpio = ngpio;
	cntr->gc.label = name;
	cntr->gc.owner = THIS_MODULE;
	cntr->gc.dev = dev;
	cntr->gc.get = mockup_gpio_get;
	cntr->gc.set = mockup_gpio_set;
	cntr->gc.direction_output = mockup_gpio_dirout;
	cntr->gc.direction_input = mockup_gpio_dirin;
	cntr->stats = devm_kzalloc(dev, sizeof(*cntr->stats) * cntr->gc.ngpio,
			GFP_KERNEL);
	if (!cntr->stats) {
		ret = -ENOMEM;
		goto err;
	}
	ret = gpiochip_add(&cntr->gc);
	if (ret)
		goto err;

	dev_info(dev, "gpio<%d..%d> add successful!", base, base + ngpio);
	return 0;
err:
	dev_err(dev, "gpio<%d..%d> add failed!", base, base + ngpio);
	return ret;
}

static int
mockup_gpio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mockup_gpio_controller *cntr;
	int ret;
	int i, j;
	int base;
	int ngpio;

	if (params_nr == 0) {
		params_nr = 2;
		conf[0] = 0;
		conf[1] = 32;
	}

	cntr = devm_kzalloc(dev, sizeof(*cntr) * params_nr, GFP_KERNEL);
	if (!cntr)
		return -ENOMEM;

	platform_set_drvdata(pdev, cntr);

	for (i = 0; i < params_nr >> 1; i++) {
		base = conf[i << 1];
		ngpio = conf[(i << 1) + 1];
		ret = mockup_gpio_add(dev, &cntr[i], pins_name[i], base, ngpio);
		if (ret) {
			dev_err(dev, "gpio<%d..%d> add failed, remove added gpio\n",
					base, base + ngpio);
			for (j = 0; j < i; j++)
				gpiochip_remove(&cntr[j].gc);

			return ret;
		}
	}

	return 0;
}

static int mockup_gpio_remove(struct platform_device *pdev)
{
	struct mockup_gpio_controller *cntr = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < params_nr >> 1; i++)
		gpiochip_remove(&cntr[i].gc);

	return 0;
}

static struct platform_driver mockup_gpio_driver = {
	.driver = {
		.name           = GPIO_NAME,
	},
	.probe = mockup_gpio_probe,
	.remove = mockup_gpio_remove,
};

static struct platform_device *pdev;
static int __init mock_device_init(void)
{
	int err;

	pdev = platform_device_alloc(GPIO_NAME, -1);
	if (!pdev)
		return -ENOMEM;

	err = platform_device_add(pdev);
	if (err) {
		platform_device_put(pdev);
		return err;
	}

	err = platform_driver_register(&mockup_gpio_driver);
	if (err) {
		platform_device_unregister(pdev);
		return err;
	}

	return 0;
}

static void __exit
mock_device_exit(void)
{
	platform_driver_unregister(&mockup_gpio_driver);
	platform_device_unregister(pdev);
}

module_init(mock_device_init);
module_exit(mock_device_exit);

MODULE_AUTHOR("Kamlakant Patel <kamlakant.patel@linaro.org>");
MODULE_AUTHOR("Bamvor Jian Zhang <bamvor.zhangjian@linaro.org>");
MODULE_DESCRIPTION("GPIO Testing driver");
MODULE_LICENSE("GPL v2");
