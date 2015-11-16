/*
 * Hwmon barebox subsystem, base class
 *
 * Copyright (C) 2015 Andrey Smirnov <andrew.smirnov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/err.h>
#include <hwmon.h>

LIST_HEAD(hwmon_sensor_list);
EXPORT_SYMBOL(hwmon_sensor_list);

int hwmon_sensor_register(struct hwmon_sensor *sensor)
{
	struct device_d *dev = &sensor->class_dev;

	if (!sensor->read)
		return -EINVAL;

	dev->id = DEVICE_ID_DYNAMIC;
	strcpy(dev->name, "sensor");
	if (sensor->dev)
		dev->parent = sensor->dev;
	platform_device_register(dev);

	list_add_tail(&sensor->list, &hwmon_sensor_list);

	return 0;
}
EXPORT_SYMBOL(hwmon_sensor_register);

int hwmon_sensor_read(struct hwmon_sensor *sensor, s32 *reading)
{
	if (!sensor->read)
		return -EINVAL;

	return sensor->read(sensor, reading);
}
EXPORT_SYMBOL(hwmon_sensor_read);
