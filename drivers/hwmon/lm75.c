/*
 * lm75.c - Part of lm_sensors, Linux kernel modules for hardware
 *	 monitoring
 * Copyright (c) 1998, 1999  Frodo Looijaard <frodol@dds.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <xfuncs.h>
#include <errno.h>
#include <i2c/i2c.h>
#include <i2c/i2c.h>
#include <linux/err.h>
#include <hwmon.h>
#include "lm75.h"


/*
 * This driver handles the LM75 and compatible digital temperature sensors.
 */

enum lm75_type {		/* keep sorted in alphabetical order */
	lm75,
};



/* The LM75 registers */
#define LM75_REG_CONF		0x01
static const u8 LM75_REG_TEMP[3] = {
	0x00,		/* input */
	0x03,		/* max */
	0x02,		/* hyst */
};

/* Each client has this additional data */
struct lm75_data {
	struct i2c_client	*client;
	struct hwmon_sensor	sensor;

	u8			resolution;	/* In bits, between 9 and 12 */
};

static inline struct lm75_data *to_lm75_data(struct hwmon_sensor *sensor)
{
	return container_of(sensor, struct lm75_data, sensor);
}

/*
 * All registers are word-sized, except for the configuration register.
 * LM75 uses a high-byte first convention, which is exactly opposite to
 * the SMBus standard.
 */
static s32 lm75_read_value(struct i2c_client *client, u8 reg)
{
	if (reg == LM75_REG_CONF)
		return i2c_smbus_read_byte_data(client, reg);
	else
		return i2c_smbus_read_word_swapped(client, reg);
}

static s32 lm75_write_value(struct i2c_client *client, u8 reg, u16 value)
{
	if (reg == LM75_REG_CONF)
		return i2c_smbus_write_byte_data(client, reg, value);
	else
		return i2c_smbus_write_word_swapped(client, reg, value);
}

static inline s32 lm75_reg_to_mc(s16 temp, u8 resolution)
{
	return ((temp >> (16 - resolution)) * 1000) >> (resolution - 8);
}

static int lm75_read_temp(struct hwmon_sensor *sensor, s32 *reading)
{
	struct lm75_data *data = to_lm75_data(sensor);

	s32 ret = lm75_read_value(data->client, LM75_REG_TEMP[0]);

	if (ret < 0)
		return ret;

	*reading = lm75_reg_to_mc((s16)ret, data->resolution);
	return 0;
}

static int lm75_probe(struct device_d *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lm75_data *data;
	int err;
	u8 set_mask, clr_mask;
	int new;
	enum lm75_type kind;

	err = dev_get_drvdata(dev, (const void **)&kind);
	if (err) {
		dev_err(dev, "Can't get driver data\n");
		return err;
	}

	data = xzalloc(sizeof(struct lm75_data));
	if (!data)
		return -ENOMEM;

	data->client = client;

	/* Set to LM75 resolution (9 bits, 1/2 degree C) and range.
	 * Then tweak to be more precise when appropriate.
	 */
	set_mask = 0;
	clr_mask = LM75_SHUTDOWN;		/* continuous conversions */

	switch (kind) {
	case lm75:
		data->resolution = 9;
		break;
	}

	/* configure as specified */
	err = lm75_read_value(client, LM75_REG_CONF);
	if (err < 0) {
		dev_err(dev, "Can't read config? %d\n", err);
		goto exit;
	}

	new = err & ~clr_mask;
	new |= set_mask;
	if (err != new)
		lm75_write_value(client, LM75_REG_CONF, new);
	dev_dbg(dev, "Config %02x\n", new);

	if (!dev->device_node) {
		dev_err(dev, "No device_node associated with the device\n");
		err = -EINVAL;
		goto exit;
	}

	data->sensor.name = of_get_property(dev->device_node,
					    "barebox,sensor-name",
					    NULL);
	if (!data->sensor.name) {
		dev_err(dev, "No sensor-name specified\n");
		err = -EINVAL;
		goto exit;
	}

	data->sensor.read = lm75_read_temp;
	data->sensor.dev = dev;

	err = hwmon_sensor_register(&data->sensor);

exit:
	if (err) {
		free(data);
		return err;
	}

	return 0;
}

static const struct platform_device_id lm75_ids[] = {
	{ "lm75", lm75, },
	{ /* LIST END */ }
};

static struct driver_d lm75_driver = {
	.name		= "lm75",
	.probe		= lm75_probe,
	.id_table	= lm75_ids,
};

static int lm75_init(void)
{
	return i2c_driver_register(&lm75_driver);
}
device_initcall(lm75_init);
