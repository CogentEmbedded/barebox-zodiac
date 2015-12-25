/*
 * GPIO based MDIO bitbang driver.
 * Supports OpenFirmware.
 *
 * (C) Copyright 2015
 *  CogentEmbedded, Andrey Gusakov <andrey.gusakov@cogentembedded.com>
 *
 * based on mvmdio driver from Linux
 *  Copyright (c) 2008 CSE Semaphore Belgium.
 *   by Laurent Pinchart <laurentp@cse-semaphore.com>
 *
 * Copyright (C) 2008, Paulius Zaleckas <paulius.zaleckas@teltonika.lt>
 *
 * Based on earlier work by
 *
 * Copyright (c) 2003 Intracom S.A.
 *  by Pantelis Antoniou <panto@intracom.gr>
 *
 * 2005 (c) MontaVista Software, Inc.
 * Vitaly Bordug <vbordug@ru.mvista.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <of_gpio.h>
#include <linux/phy.h>
#include <gpio.h>

#include "mdio-bitbang.h"

struct mdio_gpio_info {
	struct mii_bus miibus;
	struct mdiobb_ctrl ctrl;
	int mdc, mdio, mdo;
	int mdc_active_low, mdio_active_low, mdo_active_low;
};

static void mdio_dir(struct mdiobb_ctrl *ctrl, int dir)
{
	struct mdio_gpio_info *bitbang =
		container_of(ctrl, struct mdio_gpio_info, ctrl);

	if (bitbang->mdo) {
		/* Separate output pin. Always set its value to high
		 * when changing direction. If direction is input,
		 * assume the pin serves as pull-up. If direction is
		 * output, the default value is high.
		 */
		gpio_set_value(bitbang->mdo,
				1 ^ bitbang->mdo_active_low);
		return;
	}

	if (dir)
		gpio_direction_output(bitbang->mdio,
				      1 ^ bitbang->mdio_active_low);
	else
		gpio_direction_input(bitbang->mdio);
}

static int mdio_get(struct mdiobb_ctrl *ctrl)
{
	struct mdio_gpio_info *bitbang =
		container_of(ctrl, struct mdio_gpio_info, ctrl);

	return gpio_get_value(bitbang->mdio) ^
		bitbang->mdio_active_low;
}

static void mdio_set(struct mdiobb_ctrl *ctrl, int what)
{
	struct mdio_gpio_info *bitbang =
		container_of(ctrl, struct mdio_gpio_info, ctrl);

	if (bitbang->mdo)
		gpio_set_value(bitbang->mdo,
				what ^ bitbang->mdo_active_low);
	else
		gpio_set_value(bitbang->mdio,
				what ^ bitbang->mdio_active_low);
}

static void mdc_set(struct mdiobb_ctrl *ctrl, int what)
{
	struct mdio_gpio_info *bitbang =
		container_of(ctrl, struct mdio_gpio_info, ctrl);

	gpio_set_value(bitbang->mdc, what ^ bitbang->mdc_active_low);
}

static struct mdiobb_ops mdio_gpio_ops = {
	.set_mdc = mdc_set,
	.set_mdio_dir = mdio_dir,
	.set_mdio_data = mdio_set,
	.get_mdio_data = mdio_get,
};

static int mdio_gpio_probe(struct device_d *dev)
{
	int ret;
	struct device_node *np = dev->device_node;
	struct mdio_gpio_info *info;
	enum of_gpio_flags flags;

	info = xzalloc(sizeof(*info));
	dev->priv = info;

	ret = of_get_gpio_flags(np, 0, &flags);
	if (ret < 0)
		return ret;
	info->mdc = ret;
	info->mdc_active_low = flags & OF_GPIO_ACTIVE_LOW;

	ret = of_get_gpio_flags(np, 1, &flags);
	if (ret < 0)
		return ret;
	info->mdio = ret;
	info->mdio_active_low = flags & OF_GPIO_ACTIVE_LOW;

	ret = of_get_gpio_flags(np, 2, &flags);
	if (ret > 0) {
		info->mdo = ret;
		info->mdo_active_low = flags & OF_GPIO_ACTIVE_LOW;
	}

	if (gpio_request(info->mdc, "mdc"))
		return -ENODEV;

	if (gpio_request(info->mdio, "mdio"))
		return -ENODEV;

	if (info->mdo) {
		if (gpio_request(info->mdo, "mdo"))
			return -ENODEV;
		gpio_direction_output(info->mdo, 1);
		gpio_direction_input(info->mdio);
	}

	gpio_direction_output(info->mdc, 0);

	info->ctrl.ops = &mdio_gpio_ops;
	info->miibus.dev.device_node = np;
	info->miibus.priv = info;
	info->miibus.parent = dev;

	ret = init_mdio_bitbang(&info->miibus, &info->ctrl);

	return mdiobus_register(&info->miibus);
}

static void mdio_gpio_remove(struct device_d *dev)
{
	struct mdio_gpio_info *info = dev->priv;

	mdiobus_unregister(&info->miibus);
}

static const struct of_device_id gpio_mdio_dt_ids[] = {
	{ .compatible = "virtual,mdio-gpio", },
	{ /* sentinel */ }
};

static struct driver_d mdio_gpio_driver = {
	.name = "mdio-gpio",
	.probe = mdio_gpio_probe,
	.remove = mdio_gpio_remove,
	.of_compatible = DRV_OF_COMPAT(gpio_mdio_dt_ids),
};

device_platform_driver(mdio_gpio_driver);
