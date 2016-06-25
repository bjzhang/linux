/*
 * GPIO tools - utility helpers library for the GPIO tools
 *
 * Copyright (C) 2015 Linus Walleij
 *
 * Portions copied from iio_utils and lssio:
 * Copyright (c) 2010 Manuel Stahl <manuel.stahl@iis.fraunhofer.de>
 * Copyright (c) 2008 Jonathan Cameron
 * *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#ifndef _GPIO_UTILS_H_
#define _GPIO_UTILS_H_

#include <string.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static inline int check_prefix(const char *str, const char *prefix)
{
	return strlen(str) > strlen(prefix) &&
		strncmp(str, prefix, strlen(prefix)) == 0;
}

int gpio_get(const char *device_name, unsigned int line, unsigned int flag);
int gpio_gets(const char *device_name, unsigned int *lines, unsigned nlines,
	      unsigned int flag, struct gpiohandle_data *data);
int gpio_set(const char *device_name, unsigned int line, unsigned int flag,
	     unsigned int value);
int gpio_sets(const char *device_name, unsigned int *lines, unsigned int nlines,
	      unsigned int flag, struct gpiohandle_data *data);

#endif /* _GPIO_UTILS_H_ */
