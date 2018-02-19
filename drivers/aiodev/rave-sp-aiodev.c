#include <common.h>
#include <aiodev.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <of_device.h>
#include <malloc.h>
#include <linux/sizes.h>
#include <linux/mfd/rave-sp.h>

struct rave_sp_aiodev_variant {
	int (*read) (struct aiochannel *, int *);
	const char *unit;
};

struct rave_sp_aiochannel {
	struct aiochannel aiochannel;
	struct rave_sp *sp;
	unsigned int id;
};

static int rave_sp_aiodev_get(struct aiochannel *chan, u8 command,
			      void *response, size_t response_size)
{
	struct rave_sp_aiochannel *aiochannel =
		container_of(chan, struct rave_sp_aiochannel, aiochannel);
	struct rave_sp *sp = aiochannel->sp;
	u8 cmd[] = {
		[0] = command,
		[1] = 0,
		[2] = aiochannel->id,
	};

	return rave_sp_exec(sp, cmd, sizeof(cmd), response, response_size);
}

static int rave_sp_aiodev_get_temperature(struct aiochannel *chan, int *val)
{
	s16 temperature;
	int ret;

	ret = rave_sp_aiodev_get(chan, RAVE_SP_CMD_GET_TEMPERATURE,
				 &temperature, sizeof(temperature));
	if (ret)
		return ret;
	/*
	 * Returned value is in unist of 0.5 C, so we convert it into
	 * mC
	 */
	*val = (int)temperature * 500;
	return 0;
}

static int rave_sp_aiodev_get_fixed_point(struct aiochannel *chan,
					  u8 command, int *val)
{
	u8 result[2];
	int ret;

	ret = rave_sp_aiodev_get(chan, command, &result, sizeof(result));
	if (ret)
		return ret;
	/*
	 * We get result as a fixed 8.8 number with fractional part in
	 * byte [0]. It's reported in units of 1/256th, which we
	 * convert into units of 1/1000th by multiplying it by 4
	 * (1/1000 should be very close to 1/1024)
	 */
	*val = (int)result[1] * 1000 + (int)result[0] * 4;
	return 0;
}

static int rave_sp_aiodev_get_voltage(struct aiochannel *chan, int *val)
{
	return rave_sp_aiodev_get_fixed_point(chan, RAVE_SP_CMD_GET_VOLTAGE,
					      val);
}

static int rave_sp_aiodev_get_current(struct aiochannel *chan, int *val)
{
	return rave_sp_aiodev_get_fixed_point(chan, RAVE_SP_CMD_GET_CURRENT,
					      val);
}

static int rave_sp_aiodev_probe(struct device_d *dev)
{
	const struct rave_sp_aiodev_variant *variant;
	struct device_node *np = dev->device_node;
	struct rave_sp *sp = dev->parent->priv;
	struct device_node *child;
	struct aiodevice *aiodev;
	unsigned int i = 0;
	int num_channels, err;

	variant = of_device_get_match_data(dev);
	if (!variant)
		return -ENODEV;

	num_channels = of_get_child_count(np);
	if (num_channels <= 0) {
		dev_err(dev, "No channels specified via child nodes\n");
		return -EINVAL;
	}

	aiodev = xzalloc(sizeof(*aiodev));
	aiodev->name = np->name;
	aiodev->read = variant->read;
	aiodev->num_channels = num_channels;
	aiodev->channels = xmalloc(num_channels * sizeof(aiodev->channels[0]));

	for_each_child_of_node(np, child) {
		struct rave_sp_aiochannel *c;
		u32 id;

		if (of_property_read_u32(child, "reg", &id) < 0) {
			dev_warn(dev, "No \"reg\" property in %s\n",
				 child->name);
			return -EINVAL;
		}

		c = xzalloc(sizeof(*c));
		c->id = id;
		c->aiochannel.unit = variant->unit;
		c->sp = sp;
		aiodev->channels[i++] = &c->aiochannel;
	}

	err = aiodevice_register(aiodev);
	/* if (err) */
	/* 	free(data); */

	return err;
}

static const struct rave_sp_aiodev_variant rave_sp_sensor_temperature = {
	.read = rave_sp_aiodev_get_temperature,
	.unit = "mC",
};

static const struct rave_sp_aiodev_variant rave_sp_sensor_voltage = {
	.read = rave_sp_aiodev_get_voltage,
	.unit = "mV",
};

static const struct rave_sp_aiodev_variant rave_sp_sensor_current = {
	.read = rave_sp_aiodev_get_current,
	.unit = "mA",
};

static __maybe_unused const struct of_device_id rave_sp_aiodev_of_match[] = {
	{
		.compatible = "zii,rave-sp-sensor-temperature",
		.data = &rave_sp_sensor_temperature,
	},
	{
		.compatible = "zii,rave-sp-sensor-voltage",
		.data = &rave_sp_sensor_voltage,
	},
	{
		.compatible = "zii,rave-sp-sensor-current",
		.data = &rave_sp_sensor_current,
	},
	{}
};

static struct driver_d rave_sp_aiodev_driver = {
	.name = "rave-sp-aiodev",
	.probe = rave_sp_aiodev_probe,
	.of_compatible = DRV_OF_COMPAT(rave_sp_aiodev_of_match),
};
device_platform_driver(rave_sp_aiodev_driver);

