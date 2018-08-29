/*
 * EEPROM and internal MDIO driver for Marvell MV88E6XXX switches.
 *
 *   Copyright (C) 2018 Andrey Gusakiv <andrey.gusakov@cogentembedded.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <common.h>
#include <net.h>
#include <init.h>
#include <io.h>
#include <linux/err.h>
#include "mv88e6xxx.h"

/* sub-devices MDIO addresses */
#define MV88E6XXX_SWITCH_GLOBAL_REGS_1	0x1b
#define MV88E6XXX_SWITCH_GLOBAL_REGS_2	0x1c
#define MV88E6XXX_SWITCH_GLOBAL_REGS_3	0x1d


/* common */
static int mv88e6xxx_read_reg(struct mv88e6xxx_priv *priv, int chip, int reg)
{
	return mdiobus_read(priv->parent_miibus, chip, reg);
}

static int mv88e6xxx_write_reg(struct mv88e6xxx_priv *priv, int chip, int reg, u16 value)
{
	return mdiobus_write(priv->parent_miibus, chip, reg, value);
}

/* internal MDIO bus stuff */
static int mv88e6xxx_miibus_read(struct mii_bus *bus, int phy_id, int regnum)
{
	u16 reg;
	int status;
	int timeout = 100;
	struct mv88e6xxx_priv *priv = bus->priv;

	status = mv88e6xxx_read_reg(priv, MV88E6XXX_SWITCH_GLOBAL_REGS_2, 0x18);
	if (status < 0)
		return status;
	if (status & (1 << 15))
		return -EBUSY;

	reg  = regnum & 0x1f;
	reg |= (phy_id & 0x1f) << 5;
	reg |= (0x2 << 10);	/* read cmd */
	reg |= (1 << 12);	/* Clause 22 */
	reg |= (1 << 15);	/* start */

	status = mv88e6xxx_write_reg(priv, MV88E6XXX_SWITCH_GLOBAL_REGS_2, 0x18, reg);

	do {
		status = mv88e6xxx_read_reg(priv, MV88E6XXX_SWITCH_GLOBAL_REGS_2, 0x18);
	} while ((--timeout) && (status > 0) && (status & (1 << 15)));
	if (timeout == 0)
		return -ETIMEDOUT;
	if (status < 0)
		return status;

	/* read data */
	status = mv88e6xxx_read_reg(priv, MV88E6XXX_SWITCH_GLOBAL_REGS_2, 0x19);

	return status;
}

static int mv88e6xxx_miibus_write(struct mii_bus *bus, int phy_id,
			       int regnum, u16 val)
{
	u16 reg;
	int status;
	int timeout = 100;
	struct mv88e6xxx_priv *priv = bus->priv;

	status = mv88e6xxx_read_reg(priv, MV88E6XXX_SWITCH_GLOBAL_REGS_2, 0x18);
	if (status < 0)
		return status;
	if (status & (1 << 15))
		return -EBUSY;

	/* write data */
	status = mv88e6xxx_write_reg(priv, MV88E6XXX_SWITCH_GLOBAL_REGS_2, 0x19, val);
	if (status < 0)
		return status;

	reg  = regnum & 0x1f;
	reg |= (phy_id & 0x1f) << 5;
	reg |= (0x1 << 10);	/* write cmd */
	reg |= (1 << 12);	/* Clause 22 */
	reg |= (1 << 15);	/* start */

	status = mv88e6xxx_write_reg(priv, MV88E6XXX_SWITCH_GLOBAL_REGS_2, 0x18, reg);

	do {
		status = mv88e6xxx_read_reg(priv, MV88E6XXX_SWITCH_GLOBAL_REGS_2, 0x18);
	} while ((--timeout) && (status > 0) && (status & (1 << 15)));
	if (timeout == 0)
		return -ETIMEDOUT;

	return status;
}

static int mv88e6xxx_mdiibus_reset(struct mii_bus *bus)
{
	struct mv88e6xxx_priv *priv = bus->priv;

	/* TODO */
	return 0;
}

/* EEPROM stuff */
static int mv88e6xxx_read_eeprom16(struct mv88e6xxx_priv *priv, size_t off)
{
	u16 reg;
	int status;
	int timeout = 100;

	/* prepare cmd */
	reg  = (off & 0xff);
	reg |= (0x4 << 12);	/* read cmd */
	reg |= (1 << 15);	/* start operation */
	status = mv88e6xxx_write_reg(priv, MV88E6XXX_SWITCH_GLOBAL_REGS_2, 0x14, reg);
	if (status < 0)
		return status;
	do {
		status = mv88e6xxx_read_reg(priv, MV88E6XXX_SWITCH_GLOBAL_REGS_2, 0x14);
	} while ((--timeout) && (status > 0) && (status & (1 << 15)));
	if (timeout == 0)
		return -ETIMEDOUT;
	if (status < 0)
		return status;
	return mv88e6xxx_read_reg(priv, MV88E6XXX_SWITCH_GLOBAL_REGS_2, 0x15);
}

static int mv88e6xxx_write_eeprom16(struct mv88e6xxx_priv *priv, size_t off, u16 value)
{
	u16 reg;
	int status;
	int timeout = 100;

	/* prepare data */
	status = mv88e6xxx_write_reg(priv, MV88E6XXX_SWITCH_GLOBAL_REGS_2, 0x15, value);
	if (status < 0)
		return status;
	/* prepare cmd */
	reg  = (off & 0xff);
	reg |= (0x3 << 12);	/* write cmd */
	reg |= (1 << 15);	/* start operation */
	status = mv88e6xxx_write_reg(priv, MV88E6XXX_SWITCH_GLOBAL_REGS_2, 0x14, reg);
	if (status < 0)
		return status;
	do {
		status = mv88e6xxx_read_reg(priv, MV88E6XXX_SWITCH_GLOBAL_REGS_2, 0x14);
	} while ((--timeout) && (status > 0) && (status & (1 << 15)));
	if (timeout == 0)
		return -ETIMEDOUT;

	return 0;
}

static ssize_t mv88e6xxx_eeprom_cdev_read(struct cdev *cdev, void *_buf, size_t count,
		loff_t off, ulong flags)
{
	int status;
	ssize_t retval = 0;
	unsigned char *buf = _buf;
	struct mv88e6xxx_priv *priv = cdev->priv;

	if (unlikely(!count))
		return count;

	/* check if eeprom is available */
	status = mv88e6xxx_read_reg(priv, MV88E6XXX_SWITCH_GLOBAL_REGS_2, 0x14);
	if (status < 0)
		return status;
	if ((status & (1 << 11)) || (status & (1 << 15))) {
		dev_err(cdev->dev, "EEPROM interface busy: 0x%04x\n", status);
		return -ETIMEDOUT;
	}

	while (count) {
		int min;

		if ((count == 1) || (off & 1))
			min = 1;
		else
			min = 2;

		status = mv88e6xxx_read_eeprom16(priv, off >> 1);
		if (status < 0)
			return status;

		/* store data */
		if (min == 2) {
			buf[0] = status & 0xff;
			buf[1] = (status >> 8) & 0xff;
		} else if (off & 1) {
			/* first unaligned byte */
			buf[0] = (status >> 8) & 0xff;
		} else {
			/* last unaligned byte */
			buf[0] = status & 0xff;
		}

		/* adjust pointer and counters */
		buf += min;
		off += min;
		count -= min;
		retval += min;
	}

	return retval;
}

static ssize_t mv88e6xxx_eeprom_cdev_write(struct cdev *cdev, const void *_buf, size_t count,
		loff_t off, ulong flags)
{
	int status;
	ssize_t retval = 0;
	const unsigned char *buf = _buf;
	struct mv88e6xxx_priv *priv = cdev->priv;

	if (unlikely(!count))
		return count;

	/* check if eeprom is available */
	status = mv88e6xxx_read_reg(priv, MV88E6XXX_SWITCH_GLOBAL_REGS_2, 0x14);
	if (status < 0)
		return status;
	if ((status & (1 << 11)) || (status & (1 << 15))) {
		dev_err(cdev->dev, "EEPROM interface busy: 0x%04x\n", status);
		return -ETIMEDOUT;
	}
	if (!(status & (1 << 10))) {
		dev_err(cdev->dev, "EEPROM write disabled\n");
		return -EIO;
	}

	while (count) {
		int min;
		u16 value;

		/* prepare data */
		if ((count == 1) || (off & 1)) {
			min = 1;
			value = mv88e6xxx_read_eeprom16(priv, off >> 1);
			if (value < 0)
				return value;
		} else {
			min = 2;
		}

		if (min == 2)
			value  = buf[0] | (buf[1] << 8);
		else if (off & 1)
			/* first unaligned byte */
			value = (value & 0x00ff) | (buf[0] << 8);
		else
			/* last unaligned byte */
			value = (value & 0xff00) | (buf[0] << 0);

		status = mv88e6xxx_write_eeprom16(priv, off >> 1, value);
		if (status < 0)
			return status;

		/* adjust pointer and counters */
		buf += min;
		off += min;
		count -= min;
		retval += min;
	}

	return retval;
}

/* Probe */
static int mv88e6xxx_probe(struct device_d *dev)
{
	int err;
	u32 eeprom_size;
	struct mii_bus *mii;
	struct mv88e6xxx_priv *priv;
	struct mii_bus *miibus;

/*
	if (!dev->platform_data) {
		dev_err(dev, "no platform data\n");
		return -ENODEV;
	}

	pdata = dev->platform_data;
*/

	priv = xzalloc(sizeof(struct mv88e6xxx_priv));
	miibus = &priv->miibus;

	for_each_mii_bus(mii)
		if (&mii->dev == dev->parent)
			priv->parent_miibus = mii;

	if (!priv->parent_miibus) {
		dev_err(dev, "Cannot find parent MDIO bus\n");
		return -ENODEV;
	}

	priv->miibus.read = mv88e6xxx_miibus_read;
	priv->miibus.write = mv88e6xxx_miibus_write;
	priv->miibus.reset = mv88e6xxx_mdiibus_reset;
	priv->miibus.priv = priv;
	priv->miibus.parent = dev;

	mdiobus_register(miibus);

	/* EEPROM */
	if (dev->device_node &&
	    !of_property_read_u32(dev->device_node, "eeprom-size", &eeprom_size)) {
		char *devname;
		const char *alias;

		alias = of_alias_get(dev->device_node);
		if (alias) {
			devname = xstrdup(alias);
		} else {
			err = cdev_find_free_index("eeprom");
			if (err < 0) {
				dev_err(dev, "no index found to name device\n");
				goto err_unreg_mii;
			}
			devname = xasprintf("eeprom%d", err);
		}

		priv->cdev.name = devname;
		priv->cdev.priv = priv;
		priv->cdev.dev = dev;
		priv->cdev.ops = &priv->fops;
		priv->cdev.size = eeprom_size;
		priv->fops.lseek = dev_lseek_default;
		priv->fops.read	= mv88e6xxx_eeprom_cdev_read;
		priv->fops.write = mv88e6xxx_eeprom_cdev_write;
		//priv->fops.protect = mv88e6xxx_eeprom_cdev_protect;

#if 0
		if (!of_get_property(dev->device_node, "read-only", NULL)) {
			unsigned write_max = chip.page_size;

			priv->fops.write = at24_cdev_write;

			if (write_max > io_limit)
				write_max = io_limit;
			priv->write_max = write_max;

			/* buffer (data + address at the beginning) */
			priv->writebuf = xmalloc(write_max + 2);
		}
#endif
		err = devfs_create(&priv->cdev);
		if (err)
			goto err_unreg_mii;
	}

	return 0;

err_unreg_mii:
	mdiobus_unregister(miibus);

	return err;
}

static const struct of_device_id mv88e6xxx_dt_ids[] = {
	{ .compatible = "marvell,mv88e6xxx", },
	{ /* sentinel */ }
};

static struct driver_d mv88e6xxx_driver = {
	.name = "mv88e6xxx",
	.probe = mv88e6xxx_probe,
	.of_compatible = DRV_OF_COMPAT(mv88e6xxx_dt_ids),
};

device_platform_driver(mv88e6xxx_driver);
