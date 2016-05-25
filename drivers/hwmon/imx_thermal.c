/*
 * tempmon.c - handle most I2C EEPROMs
 *
 * Copyright (C) 2005-2007 David Brownell
 * Copyright (C) 2008 Wolfram Sang, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <clock.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/math64.h>
#include <linux/log2.h>
#include <linux/clk.h>
#include <mach/clock.h>
#include <mach/imx6-anadig.h>
#include <io.h>
#include <hwmon.h>

#define FACTOR0			10000000
#define MEASURE_FREQ		327
#define OCOTP_ANA1_OFFSET	(0xE * sizeof(uint32_t))

struct imx_thermal_data {
	int c1, c2;
	void __iomem *base;
	struct clk *clk;

	struct hwmon_sensor sensor;
};

static inline struct imx_thermal_data *
to_imx_thermal_data(struct hwmon_sensor *sensor)
{
	return container_of(sensor, struct imx_thermal_data, sensor);
}


static int imx_thermal_read_temperature(struct hwmon_sensor *sensor,
					s32 *reading)
{
	uint64_t start;
	uint32_t tempsense0, tempsense1;
	uint16_t n_meas;
	struct imx_thermal_data *imx_thermal = to_imx_thermal_data(sensor);

	/*
	 * now we only use single measure, every time we read
	 * the temperature, we will power on/down anadig thermal
	 * module
	 */
	writel(BM_ANADIG_TEMPSENSE0_POWER_DOWN,
	       imx_thermal->base + HW_ANADIG_TEMPSENSE0_CLR);
	writel(BM_ANADIG_ANA_MISC0_REFTOP_SELBIASOFF,
	       imx_thermal->base + HW_ANADIG_ANA_MISC0_SET);

	/* setup measure freq */
	tempsense1 = readl(imx_thermal->base + HW_ANADIG_TEMPSENSE1);
	tempsense1 &= ~BM_ANADIG_TEMPSENSE1_MEASURE_FREQ;
	tempsense1 |= MEASURE_FREQ;
	writel(tempsense1, imx_thermal->base + HW_ANADIG_TEMPSENSE1);

	/* start the measurement process */
	writel(BM_ANADIG_TEMPSENSE0_MEASURE_TEMP,
		imx_thermal->base + HW_ANADIG_TEMPSENSE0_CLR);
	writel(BM_ANADIG_TEMPSENSE0_FINISHED,
		imx_thermal->base + HW_ANADIG_TEMPSENSE0_CLR);
	writel(BM_ANADIG_TEMPSENSE0_MEASURE_TEMP,
	       imx_thermal->base + HW_ANADIG_TEMPSENSE0_SET);

	/* make sure that the latest temp is valid */
	start = get_time_ns();
	do {
		tempsense0 = readl(imx_thermal->base + HW_ANADIG_TEMPSENSE0);

		if (is_timeout(start, 1 * SECOND)) {
			dev_err(imx_thermal->sensor.dev,
				"timeout waiting for TEMPMON measurement\n");
			return -EIO;
		}
	} while (!(tempsense0 & BM_ANADIG_TEMPSENSE0_FINISHED));

	n_meas = (tempsense0 & BM_ANADIG_TEMPSENSE0_TEMP_VALUE)
		>> BP_ANADIG_TEMPSENSE0_TEMP_VALUE;
	writel(BM_ANADIG_TEMPSENSE0_FINISHED,
	       imx_thermal->base + HW_ANADIG_TEMPSENSE0_CLR);

	*reading = (int)n_meas * imx_thermal->c1 + imx_thermal->c2;

	/* power down anatop thermal sensor */
	writel(BM_ANADIG_TEMPSENSE0_POWER_DOWN,
	       imx_thermal->base + HW_ANADIG_TEMPSENSE0_SET);
	writel(BM_ANADIG_ANA_MISC0_REFTOP_SELBIASOFF,
	       imx_thermal->base + HW_ANADIG_ANA_MISC0_CLR);

	return 0;
}

static int imx_thermal_probe(struct device_d *dev)
{
	uint32_t ocotp_ana1;
	struct device_node *node;
	struct imx_thermal_data *imx_thermal;
	struct cdev *ocotp;
	char *path;
	struct device_d *anatop;
	int t1, n1, t2, n2;
	int ret;

	node = of_parse_phandle(dev->device_node, "fsl,tempmon-data", 0);
	if (!node) {
		dev_err(dev, "no calibration data source\n");
		return -ENODEV;
	}

	ret = of_find_path_by_node(node, &path, 0);
	if (ret) {
		dev_err(dev, "no OCOTP character device\n");
		return -ENODEV;
	}

	ocotp = cdev_open(path, O_RDONLY);
	if (!ocotp) {
		ret = cdev_read(ocotp,
				&ocotp_ana1, sizeof(ocotp_ana1),
				OCOTP_ANA1_OFFSET, 0);

		cdev_close(ocotp);

		if (ret != sizeof(ocotp_ana1)) {
			dev_err(dev, "failed to read calibration data\n");
			return -EINVAL;
		}
	} else {
		dev_err(dev, "failed to open %s\n", ocotp->name);
		return -EINVAL;
	}

	if (ocotp_ana1 == 0 || ocotp_ana1 == ~0) {
		dev_err(dev, "invalid sensor calibration data\n");
		return -EINVAL;
	}

	node = of_parse_phandle(dev->device_node, "fsl,tempmon", 0);
	if (!node) {
		dev_err(dev, "failed to get the value of 'fsl,tempmon'\n");
		return -ENODEV;
	}

	anatop = of_find_device_by_node(node);
	if (!anatop) {
		dev_err(dev, "failed to find 'fsl,tempmon' device\n");
		return -ENODEV;

	}

	imx_thermal = xzalloc(sizeof(*imx_thermal));

	imx_thermal->base = dev_request_mem_region(anatop, 0);
	if (IS_ERR(imx_thermal->base)) {
		ret = PTR_ERR(imx_thermal->base);
		goto fail;
	}

	n1 = ocotp_ana1 >> 20;
	t1 = 25;
	n2 = (ocotp_ana1 & 0x000FFF00) >> 8;
	t2 = ocotp_ana1 & 0xFF;

	imx_thermal->c1 = (-1000 * (t2 - t1)) / (n1 - n2);
	imx_thermal->c2 = 1000 * t2 + (1000 * n2 * (t2 - t1)) / (n1 - n2);

	imx_thermal->clk = clk_get(dev, NULL);
	if (IS_ERR(imx_thermal->clk)) {
		ret = PTR_ERR(imx_thermal->clk);
		goto fail;
	}

	clk_enable(imx_thermal->clk);

	if (!dev->device_node) {
		dev_err(dev, "No device_node associated with the device\n");
		ret = -EINVAL;
		goto disable_clock;
	}

	imx_thermal->sensor.name = of_get_property(dev->device_node,
						   "barebox,sensor-name",
						   NULL);
	if (!imx_thermal->sensor.name) {
		dev_err(dev, "No sensor-name specified\n");
		ret = -EINVAL;
		goto disable_clock;
	}

	imx_thermal->sensor.read = imx_thermal_read_temperature;
	imx_thermal->sensor.dev = dev;

	return hwmon_sensor_register(&imx_thermal->sensor);

disable_clock:
	clk_disable(imx_thermal->clk);
fail:
	kfree(imx_thermal);
	return ret;
}

static const struct of_device_id of_imx_thermal_match[] = {
	{ .compatible = "fsl,imx6q-tempmon", },
	{ .compatible = "fsl,imx6sx-tempmon", },
	{ /* end */ }
};


static struct driver_d imx_thermal_driver = {
	.name		= "imx_thermal",
	.probe		= imx_thermal_probe,
	.of_compatible	= DRV_OF_COMPAT(of_imx_thermal_match),
};

static int imx_thermal_init(void)
{
	platform_driver_register(&imx_thermal_driver);
	return 0;
}
device_initcall(imx_thermal_init);
