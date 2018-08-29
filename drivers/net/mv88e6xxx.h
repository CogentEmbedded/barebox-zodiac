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

#ifndef _MV88E6XXX_H_
#define _MV88E6XXX_H_

#include <net.h>

/*
 */
struct mv88e6xxx_priv {
	/* common */
	struct mii_bus		*parent_miibus;
	struct mii_bus 		miibus;

	/* eeprom */
	struct cdev		cdev;
	struct file_operations	fops;
};

#endif /* _MV88E6XXX_H_ */
