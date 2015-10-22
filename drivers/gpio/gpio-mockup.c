/*
 * GPIO Testing Device Driver
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/gpio/driver.h>
#include <linux/seq_file.h>
#include <linux/platform_device.h>

#define GPIO_NAME	"gpio-mockup"
#define MAX_GPIO_NO	32

/*
 * struct gpio_pin_status - structure describing a GPIO status
 * @dir:       Configures direction of gpio as "in" or "out", 0=in, 1=out
 * @value:     Configures status of the gpio as 0(low) or 1(high)
 * @int_en:    Configures interrupts status of the gpio
 */
struct gpio_pin_status {
	bool dir;
	bool value;
	bool int_en;
};

struct gpio_mockup_data {
	int ngpio;
	char *name;
};

static struct gpio_pin_status gpin_stat[MAX_GPIO_NO];

static int
mockup_gpio_get(struct gpio_chip *gc, unsigned offset)
{
	return gpin_stat[offset].value;
}

static void
mockup_gpio_set(struct gpio_chip *gc, unsigned offset, int value)
{
	if (value)
		gpin_stat[offset].value = true;
	else
		gpin_stat[offset].value = false;
}

static int
mockup_gpio_dirout(struct gpio_chip *chip, unsigned offset, int value)
{
	gpin_stat[offset].dir = true;

	return 0;
}

static int
mockup_gpio_dirin(struct gpio_chip *chip, unsigned offset)
{
	gpin_stat[offset].dir = false;

	return 0;
}

static int
mockup_gpio_probe(struct platform_device *pdev)
{
	struct gpio_mockup_data *pdata;
	struct device *dev = &pdev->dev;
	struct gpio_chip *gc;
	int ret;

	pdata = dev_get_platdata(&pdev->dev);
	if (!pdata) {
		dev_err(dev, "no platform data supplied\n");
		return -EINVAL;
	}

	gc = devm_kzalloc(&pdev->dev, sizeof(*gc), GFP_KERNEL);

	gc->base = 0;
	gc->ngpio = pdata->ngpio;
	gc->label = pdata->name;
	gc->owner = THIS_MODULE;
	gc->dev = &pdev->dev;
	gc->get = mockup_gpio_get;
	gc->set = mockup_gpio_set;
	gc->direction_output = mockup_gpio_dirout;
	gc->direction_input = mockup_gpio_dirin;

	ret = gpiochip_add(gc);
	if (ret) {
		dev_err(&pdev->dev, "gpiochip_add failed!");
		return ret;
	}

	return 0;
}

static struct platform_driver mockup_gpio_driver = {
	.driver = {
		   .name = GPIO_NAME,
		   },
	.probe = mockup_gpio_probe,
};

static struct gpio_mockup_data pdata = {
	.ngpio = 32,
	.name = GPIO_NAME,
};

static struct platform_device *dev;
static int __init
mock_device_init(void)
{
	int err;

	err = platform_driver_register(&mockup_gpio_driver);
	if (err)
		goto out;

	dev = platform_device_register_data(NULL,
					    GPIO_NAME, -1, &pdata,
					    sizeof(pdata));
	if (IS_ERR(dev))
		return PTR_ERR(dev);

	return 0;
out:
	return err;
}

static void __exit
mock_device_exit(void)
{
	platform_device_unregister(dev);
	platform_driver_unregister(&mockup_gpio_driver);
}

module_init(mock_device_init);
module_exit(mock_device_exit);

MODULE_AUTHOR("Kamlakant Patel <kamlakant.patel@linaro.org>");
MODULE_AUTHOR("Bamvor Jian Zhang <bamvor.zhangjian@linaro.org>");
MODULE_DESCRIPTION("GPIO Testing driver");
MODULE_LICENSE("GPL v2");
