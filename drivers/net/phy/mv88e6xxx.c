#include <common.h>
#include <init.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>

/* sub-devices MDIO addresses */
#define MV88E6XXX_SWITCH_GLOBAL_REGS_1		0x1b
#define MV88E6XXX_SWITCH_GLOBAL_REGS_2		0x1c
#define MV88E6XXX_SWITCH_GLOBAL_REGS_3		0x1d

/* Offset 0x00: Port Status Register */
#define MV88E6XXX_PORT_STS			0x00
#define MV88E6XXX_PORT_STS_PAUSE_EN		0x8000
#define MV88E6XXX_PORT_STS_MY_PAUSE		0x4000
#define MV88E6XXX_PORT_STS_HD_FLOW		0x2000
#define MV88E6XXX_PORT_STS_PHY_DETECT		0x1000
#define MV88E6XXX_PORT_STS_LINK			0x0800
#define MV88E6XXX_PORT_STS_DUPLEX		0x0400
#define MV88E6XXX_PORT_STS_SPEED_MASK		0x0300
#define MV88E6XXX_PORT_STS_SPEED_10		0x0000
#define MV88E6XXX_PORT_STS_SPEED_100		0x0100
#define MV88E6XXX_PORT_STS_SPEED_1000		0x0200
#define MV88E6352_PORT_STS_EEE			0x0040
#define MV88E6165_PORT_STS_AM_DIS		0x0040
#define MV88E6185_PORT_STS_MGMII		0x0040
#define MV88E6XXX_PORT_STS_TX_PAUSED		0x0020
#define MV88E6XXX_PORT_STS_FLOW_CTL		0x0010
#define MV88E6XXX_PORT_STS_CMODE_MASK		0x000f
#define MV88E6XXX_PORT_STS_CMODE_100BASE_X	0x0008
#define MV88E6XXX_PORT_STS_CMODE_1000BASE_X	0x0009
#define MV88E6XXX_PORT_STS_CMODE_SGMII		0x000a
#define MV88E6XXX_PORT_STS_CMODE_2500BASEX	0x000b
#define MV88E6XXX_PORT_STS_CMODE_XAUI		0x000c
#define MV88E6XXX_PORT_STS_CMODE_RXAUI		0x000d

/* Offset 0x01: MAC (or PCS or Physical) Control Register */
#define MV88E6XXX_PORT_MAC_CTL				0x01
#define MV88E6XXX_PORT_MAC_CTL_RGMII_DELAY_RXCLK	0x8000
#define MV88E6XXX_PORT_MAC_CTL_RGMII_DELAY_TXCLK	0x4000
#define MV88E6390_PORT_MAC_CTL_FORCE_SPEED		0x2000
#define MV88E6390_PORT_MAC_CTL_ALTSPEED			0x1000
#define MV88E6352_PORT_MAC_CTL_200BASE			0x1000
#define MV88E6XXX_PORT_MAC_CTL_FC			0x0080
#define MV88E6XXX_PORT_MAC_CTL_FORCE_FC			0x0040
#define MV88E6XXX_PORT_MAC_CTL_LINK_UP			0x0020
#define MV88E6XXX_PORT_MAC_CTL_FORCE_LINK		0x0010
#define MV88E6XXX_PORT_MAC_CTL_DUPLEX_FULL		0x0008
#define MV88E6XXX_PORT_MAC_CTL_FORCE_DUPLEX		0x0004
#define MV88E6XXX_PORT_MAC_CTL_SPEED_MASK		0x0003
#define MV88E6XXX_PORT_MAC_CTL_SPEED_10			0x0000
#define MV88E6XXX_PORT_MAC_CTL_SPEED_100		0x0001
#define MV88E6065_PORT_MAC_CTL_SPEED_200		0x0002
#define MV88E6XXX_PORT_MAC_CTL_SPEED_1000		0x0002
#define MV88E6390_PORT_MAC_CTL_SPEED_10000		0x0003
#define MV88E6XXX_PORT_MAC_CTL_SPEED_UNFORCED		0x0003

/* Offset 0x03: Switch Identifier Register */
#define MV88E6XXX_PORT_SWITCH_ID		0x03
#define MV88E6XXX_PORT_SWITCH_ID_PROD_MASK	0xfff0
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6085	0x04a0
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6095	0x0950
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6097	0x0990
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6190X	0x0a00
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6390X	0x0a10
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6131	0x1060
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6320	0x1150
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6123	0x1210
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6161	0x1610
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6165	0x1650
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6171	0x1710
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6172	0x1720
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6175	0x1750
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6176	0x1760
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6190	0x1900
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6191	0x1910
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6185	0x1a70
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6240	0x2400
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6290	0x2900
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6321	0x3100
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6141	0x3400
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6341	0x3410
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6352	0x3520
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6350	0x3710
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6351	0x3750
#define MV88E6XXX_PORT_SWITCH_ID_PROD_6390	0x3900
#define MV88E6XXX_PORT_SWITCH_ID_REV_MASK	0x000f

/* List of supported models */
enum mv88e6xxx_model {
	MV88E6085,
	MV88E6095,
	MV88E6097,
	MV88E6123,
	MV88E6131,
	MV88E6141,
	MV88E6161,
	MV88E6165,
	MV88E6171,
	MV88E6172,
	MV88E6175,
	MV88E6176,
	MV88E6185,
	MV88E6190,
	MV88E6190X,
	MV88E6191,
	MV88E6240,
	MV88E6290,
	MV88E6320,
	MV88E6321,
	MV88E6341,
	MV88E6350,
	MV88E6351,
	MV88E6352,
	MV88E6390,
	MV88E6390X,
};

enum mv88e6xxx_family {
	MV88E6XXX_FAMILY_NONE,
	MV88E6XXX_FAMILY_6065,	/* 6031 6035 6061 6065 */
	MV88E6XXX_FAMILY_6095,	/* 6092 6095 */
	MV88E6XXX_FAMILY_6097,	/* 6046 6085 6096 6097 */
	MV88E6XXX_FAMILY_6165,	/* 6123 6161 6165 */
	MV88E6XXX_FAMILY_6185,	/* 6108 6121 6122 6131 6152 6155 6182 6185 */
	MV88E6XXX_FAMILY_6320,	/* 6320 6321 */
	MV88E6XXX_FAMILY_6341,	/* 6141 6341 */
	MV88E6XXX_FAMILY_6351,	/* 6171 6175 6350 6351 */
	MV88E6XXX_FAMILY_6352,	/* 6172 6176 6240 6352 */
	MV88E6XXX_FAMILY_6390,  /* 6190 6190X 6191 6290 6390 6390X */
};

struct mv88e6xxx_info {
	enum mv88e6xxx_family family;
	u16 prod_num;
	const char *name;
	unsigned int num_ports;
	unsigned int port_base_addr;
};

static const struct mv88e6xxx_info mv88e6xxx_table[] = {
	[MV88E6085] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6085,
		.family = MV88E6XXX_FAMILY_6097,
		.name = "Marvell 88E6085",
		.num_ports = 10,
		.port_base_addr = 0x10,
	},

	[MV88E6095] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6095,
		.family = MV88E6XXX_FAMILY_6095,
		.name = "Marvell 88E6095/88E6095F",
		.num_ports = 11,
		.port_base_addr = 0x10,
	},

	[MV88E6097] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6097,
		.family = MV88E6XXX_FAMILY_6097,
		.name = "Marvell 88E6097/88E6097F",
		.num_ports = 11,
		.port_base_addr = 0x10,
	},

	[MV88E6123] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6123,
		.family = MV88E6XXX_FAMILY_6165,
		.name = "Marvell 88E6123",
		.num_ports = 3,
		.port_base_addr = 0x10,
	},

	[MV88E6131] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6131,
		.family = MV88E6XXX_FAMILY_6185,
		.name = "Marvell 88E6131",
		.num_ports = 8,
		.port_base_addr = 0x10,

	},

	[MV88E6141] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6141,
		.family = MV88E6XXX_FAMILY_6341,
		.name = "Marvell 88E6341",
		.num_ports = 6,
		.port_base_addr = 0x10,

	},

	[MV88E6161] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6161,
		.family = MV88E6XXX_FAMILY_6165,
		.name = "Marvell 88E6161",
		.num_ports = 6,
		.port_base_addr = 0x10,

	},

	[MV88E6165] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6165,
		.family = MV88E6XXX_FAMILY_6165,
		.name = "Marvell 88E6165",
		.num_ports = 6,
		.port_base_addr = 0x10,

	},

	[MV88E6171] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6171,
		.family = MV88E6XXX_FAMILY_6351,
		.name = "Marvell 88E6171",
		.num_ports = 7,
		.port_base_addr = 0x10,

	},

	[MV88E6172] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6172,
		.family = MV88E6XXX_FAMILY_6352,
		.name = "Marvell 88E6172",
		.num_ports = 7,
		.port_base_addr = 0x10,

	},

	[MV88E6175] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6175,
		.family = MV88E6XXX_FAMILY_6351,
		.name = "Marvell 88E6175",
		.num_ports = 7,
		.port_base_addr = 0x10,

	},

	[MV88E6176] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6176,
		.family = MV88E6XXX_FAMILY_6352,
		.name = "Marvell 88E6176",
		.num_ports = 7,
		.port_base_addr = 0x10,

	},

	[MV88E6185] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6185,
		.family = MV88E6XXX_FAMILY_6185,
		.name = "Marvell 88E6185",
		.num_ports = 10,
		.port_base_addr = 0x10,

	},

	[MV88E6190] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6190,
		.family = MV88E6XXX_FAMILY_6390,
		.name = "Marvell 88E6190",
		.num_ports = 11,	/* 10 + Z80 */
		.port_base_addr = 0x0,

	},

	[MV88E6190X] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6190X,
		.family = MV88E6XXX_FAMILY_6390,
		.name = "Marvell 88E6190X",
		.num_ports = 11,	/* 10 + Z80 */
		.port_base_addr = 0x0,

	},

	[MV88E6191] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6191,
		.family = MV88E6XXX_FAMILY_6390,
		.name = "Marvell 88E6191",
		.num_ports = 11,	/* 10 + Z80 */
		.port_base_addr = 0x0,

	},

	[MV88E6240] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6240,
		.family = MV88E6XXX_FAMILY_6352,
		.name = "Marvell 88E6240",
		.num_ports = 7,
		.port_base_addr = 0x10,

	},

	[MV88E6290] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6290,
		.family = MV88E6XXX_FAMILY_6390,
		.name = "Marvell 88E6290",
		.num_ports = 11,	/* 10 + Z80 */
		.port_base_addr = 0x0,

	},

	[MV88E6320] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6320,
		.family = MV88E6XXX_FAMILY_6320,
		.name = "Marvell 88E6320",
		.num_ports = 7,
		.port_base_addr = 0x10,

	},

	[MV88E6321] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6321,
		.family = MV88E6XXX_FAMILY_6320,
		.name = "Marvell 88E6321",
		.num_ports = 7,
		.port_base_addr = 0x10,
	},

	[MV88E6341] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6341,
		.family = MV88E6XXX_FAMILY_6341,
		.name = "Marvell 88E6341",
		.num_ports = 6,
		.port_base_addr = 0x10,
	},

	[MV88E6350] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6350,
		.family = MV88E6XXX_FAMILY_6351,
		.name = "Marvell 88E6350",
		.num_ports = 7,
		.port_base_addr = 0x10,
	},

	[MV88E6351] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6351,
		.family = MV88E6XXX_FAMILY_6351,
		.name = "Marvell 88E6351",
		.num_ports = 7,
		.port_base_addr = 0x10,
	},

	[MV88E6352] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6352,
		.family = MV88E6XXX_FAMILY_6352,
		.name = "Marvell 88E6352",
		.num_ports = 7,
		.port_base_addr = 0x10,
	},

	[MV88E6390] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6390,
		.family = MV88E6XXX_FAMILY_6390,
		.name = "Marvell 88E6390",
		.num_ports = 11,	/* 10 + Z80 */
		.port_base_addr = 0x0,
	},

	[MV88E6390X] = {
		.prod_num = MV88E6XXX_PORT_SWITCH_ID_PROD_6390X,
		.family = MV88E6XXX_FAMILY_6390,
		.name = "Marvell 88E6390X",
		.num_ports = 11,	/* 10 + Z80 */
		.port_base_addr = 0x0,
	},
};

struct mv88e6xxx_priv {
	/* common */
	struct mii_bus		*parent_miibus;
	const struct mv88e6xxx_info	*info;

	/* internal mii bus */
	struct mii_bus 		miibus;

	/* eeprom */
	struct cdev		cdev;
	struct file_operations	fops;
};

static int mv88e6xxx_port_config_init(struct phy_device *phydev)
{
	int mac_ctl, err;

	if ((phydev->interface == PHY_INTERFACE_MODE_RGMII) ||
	    (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID) ||
	    (phydev->interface == PHY_INTERFACE_MODE_RGMII_RXID) ||
	    (phydev->interface == PHY_INTERFACE_MODE_RGMII_TXID)) {
		mac_ctl = phy_read(phydev, MV88E6XXX_PORT_MAC_CTL);

		if (mac_ctl < 0)
			return mac_ctl;

		mac_ctl &= ~(MV88E6XXX_PORT_MAC_CTL_RGMII_DELAY_RXCLK |
			     MV88E6XXX_PORT_MAC_CTL_RGMII_DELAY_TXCLK);

		if (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID ||
		    phydev->interface == PHY_INTERFACE_MODE_RGMII_TXID)
			mac_ctl |= MV88E6XXX_PORT_MAC_CTL_RGMII_DELAY_TXCLK;

		if (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID ||
		    phydev->interface == PHY_INTERFACE_MODE_RGMII_RXID)
			mac_ctl |= MV88E6XXX_PORT_MAC_CTL_RGMII_DELAY_RXCLK;

		err = phy_write(phydev, MV88E6XXX_PORT_MAC_CTL, mac_ctl);
		if (err < 0)
			return err;
	}

	return 0;
}

static int mv88e6xxx_port_config_aneg(struct phy_device *phydev)
{
	return 0;
}

static int mv88e6xxx_port_read_status(struct phy_device *phydev)
{
	const int sts = phy_read(phydev, MV88E6XXX_PORT_STS);

	if (sts < 0)
		return sts;

	phydev->link = !!(sts & MV88E6XXX_PORT_STS_LINK);

	if (sts & MV88E6XXX_PORT_STS_DUPLEX)
		phydev->duplex = DUPLEX_FULL;
	else
		phydev->duplex = DUPLEX_HALF;

	switch (sts & MV88E6XXX_PORT_STS_SPEED_MASK) {
	case MV88E6XXX_PORT_STS_SPEED_1000:
		phydev->speed = SPEED_1000;
		break;

	case MV88E6XXX_PORT_STS_SPEED_100:
		phydev->speed = SPEED_100;
		break;

	default:
		phydev->speed = SPEED_10;
		break;
	};

	/*
	 * FIXME: Probably wrong
	 */
	phydev->pause = phydev->asym_pause = 0;

	return 0;
}

static struct phy_driver mv88e6xxx_port_driver = {
	.drv.name	= "Marvel 88E6xxx Port",
	.features	= PHY_GBIT_FEATURES,
	.config_init	= mv88e6xxx_port_config_init,
	.config_aneg	= mv88e6xxx_port_config_aneg,
	.read_status	= mv88e6xxx_port_read_status,
};

/* common helpers for eerom and internal mii bus */
static int mv88e6xxx_read_reg(struct mv88e6xxx_priv *priv, int addr, int regnum)
{
	return mdiobus_read(priv->parent_miibus, addr, regnum);
}

static int mv88e6xxx_write_reg(struct mv88e6xxx_priv *priv, int addr, int regnum, u16 value)
{
	return mdiobus_write(priv->parent_miibus, addr, regnum, value);
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

static const struct mv88e6xxx_info *mv88e6xxx_lookup_info(unsigned int prod_num)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mv88e6xxx_table); ++i)
		if (mv88e6xxx_table[i].prod_num == prod_num)
			return &mv88e6xxx_table[i];

	return NULL;
}

static const struct mv88e6xxx_info *mv88e6xxx_detect(struct device_d *dev,
						     struct mii_bus *bus)
{
	const struct mv88e6xxx_info *info;
	unsigned int prod_num, rev;
	int id;

	id = mdiobus_read(bus,
			  mv88e6xxx_table[MV88E6085].port_base_addr,
			  MV88E6XXX_PORT_SWITCH_ID);
	if (id < 0)
		return ERR_PTR(id);

	prod_num = id & MV88E6XXX_PORT_SWITCH_ID_PROD_MASK;
	rev = id & MV88E6XXX_PORT_SWITCH_ID_REV_MASK;

	info = mv88e6xxx_lookup_info(prod_num);
	if (!info)
		return ERR_PTR(-ENODEV);

	dev_info(dev, "switch 0x%x detected: %s, revision %u\n",
		 info->prod_num, info->name, rev);

	return info;
}

static int mv88e6xxx_probe(struct device_d *dev)
{
	int err;
	u32 eeprom_size;
	struct device_node *switch_node, *port_node, *mdio_node;
	struct mv88e6xxx_priv *priv;

	priv = xzalloc(sizeof(struct mv88e6xxx_priv));

	priv->parent_miibus = of_mdio_find_bus(dev->device_node->parent);
	if (!priv->parent_miibus)
		return -EPROBE_DEFER;

	priv->info = mv88e6xxx_detect(dev, priv->parent_miibus);
	if (IS_ERR(priv->info)) {
		dev_err(dev, "Error: Failed to detect switch type\n");
		return PTR_ERR(priv->info);
	}

	/* register ports */
	switch_node = of_find_node_by_name(dev->device_node, "ports");
	if (!switch_node)
		return -EINVAL;

	for_each_available_child_of_node(switch_node, port_node) {
		struct phy_device *phydev;
		u32 nr;
		int r;

		r = of_property_read_u32(port_node, "reg", &nr);
		if (r) {
			dev_err(dev,
				"Error: Failed to find reg for child %s\n",
				port_node->full_name);
			continue;
		}

		if (nr >= priv->info->num_ports) {
			dev_err(dev, "Error: Incorrect port number %d\n", nr);
			continue;
		}

		phydev = phy_device_create(priv->parent_miibus,
					   priv->info->port_base_addr + nr, 0);
		phydev->dev.driver      = &mv88e6xxx_port_driver.drv;
		phydev->dev.device_node = port_node;
		phydev->autoneg         = AUTONEG_DISABLE;

		r = phy_register_device(phydev);
		if (r) {
			dev_err(dev, "Error: Failed to register a PHY\n");
		}
	}

	/* internal MII bus */
	mdio_node = of_find_node_by_name(dev->device_node, "mdio");
	if (mdio_node) {
		priv->miibus.read = mv88e6xxx_miibus_read;
		priv->miibus.write = mv88e6xxx_miibus_write;
		/* only first 5 are phys */
		//priv->miibus.phy_mask = 0xffffffe0;
		priv->miibus.priv = priv;
		priv->miibus.parent = dev;

		mdiobus_register(&priv->miibus);
	} else {
		dev_err(dev, "No MDIO node\n");
	}

	/* EEPROM */
	if (/* TODO: check for eeprom access support in switch */
	    of_property_read_u32(dev->device_node, "eeprom-size", &eeprom_size)) {
		char *devname;
		const char *alias;

		alias = of_alias_get(dev->device_node);
		if (alias) {
			devname = xstrdup(alias);
		} else {
			err = cdev_find_free_index("eeprom");
			if (err < 0) {
				dev_err(dev, "no index found to name device\n");
				return err;
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

		err = devfs_create(&priv->cdev);
	}

	return 0;
}

static const struct of_device_id mv88e6xxx_of_match[] = {
	{
		.compatible = "marvell,mv88e6085",
	},
	{},
};

static struct driver_d mv88e6xxx_driver = {
	.name	       = "mv88e6085",
	.probe         = mv88e6xxx_probe,
	.of_compatible = mv88e6xxx_of_match,
};
device_platform_driver(mv88e6xxx_driver);
