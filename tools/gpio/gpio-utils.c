/*
 * GPIO tools - helpers library for the GPIO tools
 *
 * Copyright (C) 2015 Linus Walleij
 * Copyright (C) 2016 Bamvor Jian Zhang
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>
#include "gpio-utils.h"

int gpio_op(const char *device_name, unsigned int *lines, unsigned nlines,
	      unsigned int flag, struct gpiohandle_data *data, int request)
{
	struct gpiohandle_request req;
	char *chrdev_name;
	int fd;
	int i;
	int ret;

	ret = asprintf(&chrdev_name, "/dev/%s", device_name);
	if (ret < 0)
		return -ENOMEM;

	fd = open(chrdev_name, 0);
	if (fd == -1) {
		ret = -errno;
		fprintf(stderr, "Failed to open %s\n", chrdev_name);
		goto exit_close_error;
	}

	for ( i = 0; i < nlines; i++ )
		req.lineoffsets[i] = lines[i];

	req.flags = flag;
	strcpy(req.consumer_label, "gpio-util");
	req.lines = nlines;
	ret = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &req);
	if (ret == -1) {
		ret = -errno;
		fprintf(stderr, "Failed to issue GET LINEHANDLE "
			"IOCTL (%d)\n",
			ret);
		goto exit_close_error;
	}

	ret = ioctl(req.fd, request, data);
	if (ret == -1) {
		ret = -errno;
		fprintf(stderr, "Failed to issue GPIOHANDLE GET LINE "
			"VALUES IOCTL (%d)\n",
			ret);
		goto exit_close_error;
	}

	if (close(req.fd) == -1)
		perror("Failed to close GPIOHANDLE file");
exit_close_error:
	if (close(fd) == -1)
		perror("Failed to close GPIO character device file");
	free(chrdev_name);
	return ret;
}

int gpio_gets(const char *device_name, unsigned int *lines, unsigned nlines,
	      unsigned int flag, struct gpiohandle_data *data)
{
	return gpio_op(device_name, lines, nlines, flag, data,
		       GPIOHANDLE_GET_LINE_VALUES_IOCTL);
}

int gpio_sets(const char *device_name, unsigned int *lines, unsigned nlines,
	      unsigned int flag, struct gpiohandle_data *data)
{
	return gpio_op(device_name, lines, nlines, flag, data,
		       GPIOHANDLE_SET_LINE_VALUES_IOCTL);
}

int gpio_get(const char *device_name, unsigned int line, unsigned int flag)
{
	struct gpiohandle_data data;
	unsigned int lines[] = {line};

	gpio_gets(device_name, lines, 1, flag, &data);
	return data.values[0];
}

int gpio_set(const char *device_name, unsigned int line, unsigned int flag,
	     unsigned int value)
{
	struct gpiohandle_data data;
	unsigned int lines[] = {line};

	data.values[0] = value;
	return gpio_sets(device_name, lines, 1, flag, &data);
}

