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
//#include <linux/debugfs.h>
#include <linux/gpio/driver.h>
//#include <linux/seq_file.h>
#include <linux/platform_device.h>

#define GPIO_NAME	"gpio-mockup"
#define	MAX_GC		9

typedef enum {
	IN,
	OUT
} direction;

/*
 * struct gpio_pin_status - structure describing a GPIO status
 * @dir:       Configures direction of gpio as "in" or "out", 0=in, 1=out
 * @value:     Configures status of the gpio as 0(low) or 1(high)
 */
struct gpio_pin_status {
       direction	dir;
       int		value;
};

struct mockup_gpio_controller {
	struct gpio_chip	gc;
	struct gpio_pin_status	*stats;
};

///*
// * struct mockup_gpio_data - structure describing a GPIO mockup parameter
// * @base:      Configures the base of this gpio chip
// * @ngpio:     Configures the number of gpio chip
// */
////TODO: provide base and ngpio in order to test overlap checking logic.
//struct mockup_gpio_data{
//	int base;
//	int ngpio;
//};

int mockup_gpio[MAX_GC << 1];
int gc_nr;
module_param_array(mockup_gpio, int, &gc_nr, 0400);

static int mockup_gpio_get(struct gpio_chip *gc, unsigned offset)
{
	struct mockup_gpio_controller *cntr = container_of(gc,
			struct mockup_gpio_controller, gc);
	return cntr->stats[offset].value;
}

static void mockup_gpio_set(struct gpio_chip *gc, unsigned offset, int value)
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
	cntr->stats[offset].dir= OUT;

	return 0;
}

static int mockup_gpio_dirin(struct gpio_chip *gc, unsigned offset)
{
	struct mockup_gpio_controller *cntr = container_of(gc,
			struct mockup_gpio_controller, gc);
	cntr->stats[offset].dir= IN;

	return 0;
}


int mockup_gpio_add(struct device *dev, struct mockup_gpio_controller *cntr, int base, int ngpio)
{
	int ret;

	printk("base: %d, ngpio: %d\n", base, ngpio);
	cntr->gc.base = base;
	cntr->gc.ngpio = ngpio;
	//TODO: name-%i
	cntr->gc.label = GPIO_NAME;
	cntr->gc.owner = THIS_MODULE;
	cntr->gc.dev = dev;
	cntr->gc.get = mockup_gpio_get;
	cntr->gc.set = mockup_gpio_set;
	cntr->gc.direction_output = mockup_gpio_dirout;
	cntr->gc.direction_input = mockup_gpio_dirin;
	ret = gpiochip_add(&cntr->gc);
	if (ret) {
	       dev_err(dev, "gpiochip_add failed!");
	       return ret;
	}
	cntr->stats = devm_kzalloc(dev, sizeof(*cntr->stats) * cntr->gc.ngpio, GFP_KERNEL);
	if (!cntr->stats)
		return -ENOMEM;

	return 0;
}

static int mockup_gpio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mockup_gpio_controller *cntr;
	int ret;
	int i, j;

	if ( gc_nr == 0 ) {
		gc_nr = 1;
		mockup_gpio[0] = 0;
		mockup_gpio[1] = 32;
	}

	cntr = devm_kzalloc(dev, sizeof(*cntr) * gc_nr, GFP_KERNEL);
	if (!cntr)
		return -ENOMEM;

	platform_set_drvdata(pdev, cntr);

	printk("gc_nr: %d\n", gc_nr);

	for ( i = 0; i < gc_nr >> 1; i++ ) {
		ret = mockup_gpio_add(dev, &cntr[i], mockup_gpio[i << 1], mockup_gpio[(i << 1) + 1]);
		if (ret) {
			for ( j = 0; j < i; j++ ) {
				gpiochip_remove(&cntr[j].gc);
			}
			return ret;
		}
	}

	dev_info(dev, "mockup gpio add successful\n");
	return 0;
}

static int mockup_gpio_remove(struct platform_device *pdev)
{
	int i;

	struct mockup_gpio_controller *cntr = platform_get_drvdata(pdev);
	for ( i = 0; i < gc_nr >> 1; i++ ) {
		gpiochip_remove(&cntr[i].gc);
	}

	return 0;
}

static struct platform_driver mockup_gpio_driver = {
       .driver = {
               .name           = GPIO_NAME,
       },
       .probe = mockup_gpio_probe,
       .remove= mockup_gpio_remove,
};

static struct platform_device *dev;
static int __init mock_device_init(void)
{
	int err;

	err = platform_driver_register(&mockup_gpio_driver);
	if (err)
		goto err_drv_reg;

	dev = platform_device_alloc(GPIO_NAME, -1);
	if ( !dev ) {
		err = ENOMEM;
		goto err_dev_alloc;
	}

	err = platform_device_add(dev);
	if (err) {
	       goto err_dev_reg;
	}

	return 0;

err_dev_reg:
err_dev_alloc:
	platform_driver_unregister(&mockup_gpio_driver);
err_drv_reg:
	return err;
}

static void __exit mock_device_exit(void)
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
