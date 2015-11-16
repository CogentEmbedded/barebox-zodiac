#ifndef __HWMON_H__
#define __HWMON_H__

#include <common.h>
#include <driver.h>
#include <linux/types.h>

enum hwmon_sensor_type {
	SENSOR_TEMPERATURE,
};

struct hwmon_sensor {
	int (*read) (struct hwmon_sensor *, s32 *);
	const char *name;

	struct device_d *dev;
	struct device_d class_dev;

	enum hwmon_sensor_type type;

	struct list_head list;
};


int hwmon_sensor_register(struct hwmon_sensor *sensor);
int hwmon_sensor_read(struct hwmon_sensor *sensor, s32 *reading);

extern struct list_head hwmon_sensor_list;
#define for_each_hwmon_sensor(sensor) list_for_each_entry(sensor, &hwmon_sensor_list, list)


#endif
