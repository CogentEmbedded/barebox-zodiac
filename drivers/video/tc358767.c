/*
 *
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
#include <of_gpio.h>
#include <gpio.h>

/* registers */
#define TC_IDREG	0x0500

#define DP0CTL		0x0600
#define DP0_AUXCFG0	0x0660
#define DP0_AUXCFG1	0x0664
#define DP0_AUXADDR	0x0668
#define DP0_AUXWDATA(i)	(0x066c + (i) * 4)
#define DP0_AUXRDATA(i)	(0x067c + (i) * 4)
#define DP0_AUXSTATUS	0x068c
#define DP0_AUXI2CADR	0x0698

#define DP0_SRCCTRL	0x06a0

#define DP0_LTSTAT	0x06d0
#define DP0_LTLOOPCTRL	0x06d8
#define DP0_SNKLTCTRL	0x06e4

#define DP_PHY_CTRL	0x0800
#define DP0_PLLCTRL	0x0900
#define PXL_PLLCTRL	0x0908
#define PXL_PLLPARAM	0x0914
#define SYS_PLLPARAM	0x0918

struct tc_data {
	struct i2c_client	*client;
	struct device_d		*dev;
	/* aux i2c */
	struct i2c_adapter	adapter;

	int enable_gpio;
	int enable_active_high;
	int reset_gpio;
	int reset_active_high;
};
#define to_tc_i2c_struct(a)	container_of(a, struct tc_data, adapter)


static int tc_write_reg(struct tc_data *data, u16 reg, u32 value)
{
	int ret;
	u8 buf[4];

	buf[0] = value & 0xff;
	buf[1] = (value >> 8) & 0xff;
	buf[2] = (value >> 16) & 0xff;
	buf[3] = (value >> 24) & 0xff;

	ret = i2c_write_reg(data->client, reg | I2C_ADDR_16_BIT, buf, 4);
	if (ret != 4) {
		dev_err(data->dev, "error writing reg 0x%04x: %d\n",
			reg, ret);
		return ret;
	}

	return 0;
}


static int tc_read_reg(struct tc_data *data, u16 reg, u32 *value)
{
	int ret;
	u8 buf[4];

	ret = i2c_read_reg(data->client, reg | I2C_ADDR_16_BIT, buf, 4);
	if (ret != 4) {
		dev_err(data->dev, "error reading reg 0x%04x: %d\n",
			reg, ret);
		return ret;
	}

	*value = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);

	return 0;
}

/* simple macros to avoid error checks */
#define tc_write(reg, var)	do { 		\
	ret = tc_write_reg(tc, reg, var);	\
	if (ret)				\
		goto err;			\
	} while (0)
#define tc_read(reg, var)	do {		\
	ret = tc_read_reg(tc, reg, var);	\
	if (ret)				\
		goto err;			\
	} while (0)

static int tc_aux_i2c_get_status(struct tc_data *tc)
{
	int ret;
	u32 value;

	tc_read(DP0_AUXSTATUS, &value);
	if ((value & 0x01) == 0x00) {
		switch (value & 0xf0) {
		case 0x00:
			//printf("Ack! 0x%08x\n", value);
			return 0;
		case 0x40:
			//printf("Nack! 0x%08x\n", value);
			return -EIO;
		case 0x80:
			printf("Defer! 0x%08x\n", value);
			return -EAGAIN;
		}
		return 0;
	}

	if (value & 0x02) {
		printf("Timeout!\n");
		return -ETIME;
	}
	return -EBUSY;
err:
	return ret;
}

static int tc_aux_i2c_wait_busy(struct tc_data *tc, int delay)
{
	int ret;
	u32 value;

	do {
		tc_read(DP0_AUXSTATUS, &value);
		if ((value & 0x01) == 0x00)
			return 0;
		mdelay(1);
	} while (delay--);

	return -EBUSY;
err:
	return ret;
}

static int tc_aux_i2c_write(struct tc_data *tc, struct i2c_msg *msg)
{
	int i = 0;
	int ret;
	u32 tmp = 0;
	
	if (msg->flags & I2C_M_DATA_ONLY)
		return -EINVAL;

	ret = tc_aux_i2c_wait_busy(tc, 100);
	if (ret)
		goto err;

	/* store data */
	while (i < msg->len) {
		/* check endian!!! */
		tmp = (tmp << 8) | msg->buf[i];
		i++;
		if (((i % 4) == 0) ||
		    (i == msg->len)) {
			tc_write(DP0_AUXWDATA(i >> 2), tmp);
			tmp = 0;
		}
	}
	/* store address */
	tc_write(DP0_AUXADDR, msg->addr);
	/* start transfer */
	tc_write(DP0_AUXCFG0, (msg->len << 8) | 0x00);

	ret = tc_aux_i2c_wait_busy(tc, 100);
	if (ret)
		goto err;

	return tc_aux_i2c_get_status(tc);

err:
	return ret;
}

static int tc_aux_i2c_read(struct tc_data *tc, struct i2c_msg *msg)
{
	int i = 0;
	int ret;
	u32 tmp;
	
	if (msg->flags & I2C_M_DATA_ONLY)
		return -EINVAL;

	ret = tc_aux_i2c_wait_busy(tc, 100);
	if (ret)
		goto err;

	/* store address */
	tc_write(DP0_AUXADDR, msg->addr);
	/* start transfer */
	tc_write(DP0_AUXCFG0, (msg->len << 8) | 0x01);

	ret = tc_aux_i2c_wait_busy(tc, 100);
	if (ret)
		goto err;

	ret = tc_aux_i2c_get_status(tc);
	if (ret)
		goto err;

	/* read data */
	while (i < msg->len) {
		if ((i % 4) == 0)
			tc_read(DP0_AUXRDATA(i >> 2), &tmp);
		msg->buf[i] = tmp & 0xFF;
		tmp = tmp >> 8;
		i++;
	}

	return 0;
err:
	return ret;
}

static int tc_aux_i2c_xfer(struct i2c_adapter *adapter,
			struct i2c_msg *msgs, int num)
{
	struct tc_data *data = to_tc_i2c_struct(adapter);
	unsigned int i;
	int ret;

	/* check */
	for (i = 0; i < num; i++) {
		if (msgs[i].len > 16) {
			dev_err(data->dev, "this bus support max 16 bytes per transfer\n");
			return -EINVAL;
		}
	}

	/* read/write data */
	for (i = 0; i < num; i++) {
		/* write/read data */
		if (msgs[i].flags & I2C_M_RD)
			ret = tc_aux_i2c_read(data, &msgs[i]);
		else
			ret = tc_aux_i2c_write(data, &msgs[i]);
		if (ret)
			goto err;
	}

err:
	return (ret < 0) ? ret : num;
}

static int tc_setup(struct tc_data *tc)
{
	int ret;
	u32 value;


	/* ----Setup Main Link-------- */
	tc_write(DP0_SRCCTRL, 0x00003087);
	//DP0_SrcCtrl	0x06A0	0x00003083
	//tc_write(0x07A0, 0x00003083);	//???
	tc_write(SYS_PLLPARAM, 0x00000101);

	/* ----Setup DP-PHY / PLL------- */
	tc_write(DP_PHY_CTRL, 0x03000007);
	tc_write(DP0_PLLCTRL, 0x00000005);
	/* wait PLL lock */
	mdelay(100);
	//#DP1_PLLCTRL	0x0904	0x00000005
	tc_write(0x0904, 0x00000005);	//???
	tc_write(PXL_PLLPARAM, 0x01330141);
	tc_write(PXL_PLLCTRL, 0x00000005);

	/* ----Reset/Enable Main Links-------- */
	tc_write(DP_PHY_CTRL, 0x13001107);

	tc_write(DP_PHY_CTRL, 0x03000007);
	mdelay(1000); /* ??? */

	tc_read(DP_PHY_CTRL, &value);
	printk("Reg DP_PHY_CTRL 0x%08x (should be 0x%08x)\n",
		value, 0x03010007);

	/* ----Read DP Rx Link Capability-------- */
	tc_write(DP0_AUXCFG1, 0x0001063F);
	tc_write(DP0_AUXADDR, 0x00000001);
	tc_write(DP0_AUXCFG0, 0x00000009);

	tc_read(DP0_AUXSTATUS, &value);
	printk("Reg DP0_AUXSTATUS 0x%08x (should be 0x%08x)\n",
		value, 0x00000100);

	tc_read(DP0_AUXRDATA(0), &value);
	printk("Reg DP0_AUXRDATA0 0x%08x (should be 0x%08x)\n",
		value, 0x0A);

	tc_write(DP0_AUXADDR, 0x00000002);
	tc_write(DP0_AUXCFG0, 0x00000009);

	tc_read(DP0_AUXSTATUS, &value);
	printk("Reg DP0_AUXSTATUS 0x%08x (should be 0x%08x)\n",
		value, 0x00000100);

	tc_read(DP0_AUXRDATA(0), &value);
	printk("Reg DP0_AUXRDATA0 0x%08x (should be 0x02/0x82)\n",
		value);

	/* ----Setup Link & DPRx Config for Training-------- */
	tc_write(DP0_AUXADDR, 0x00000100);

	tc_write(DP0_AUXWDATA(0), 0x0000020A);
	tc_write(DP0_AUXCFG0, 0x00000108);
	tc_write(DP0_AUXADDR, 0x00000108);
	tc_write(DP0_AUXWDATA(0), 0x00000001);
	tc_write(DP0_AUXCFG0, 0x00000008);

	/* ----Set DPCD 00102h for Training Pat 1-------- */
	tc_write(DP0_SNKLTCTRL, 0x00000021);
	tc_write(DP0_LTLOOPCTRL, 0xF600000D);

	/* ----Set DP0 Trainin Pattern 1-------- */
	tc_write(DP0_SRCCTRL, 0x00003187);

	/* ----Enable DP0 to start Link Training-------- */
	tc_write(DP0CTL, 0x00000001);

	tc_read(DP0_LTSTAT, &value);
	printk("Reg DP0_LTSTAT 0x%08x (should be 0x%08x)\n",
		value, 0x00002811);

	/* ----Set DPCD 00102h for Link Traing Pat 2-------- */
	tc_write(DP0_SNKLTCTRL, 0x00000022);

	/* ----Set DP0 Trainin Pattern 2-------- */
	tc_write(DP0_SRCCTRL, 0x00003287);

	tc_read(DP0_LTSTAT, &value);
	printk("Reg DP0_LTSTAT 0x%08x (should be 0x%08x)\n",
		value, 0x0000307F);

	/* ----Clear DPCD 00102h-------- */
	tc_write(DP0_AUXADDR, 0x00000102);
	tc_write(DP0_AUXWDATA(0), 0x00000000);
	tc_write(DP0_AUXCFG0, 0x00000008);

	/* ----Clear DP0 Training Pattern-------- */
	tc_write(DP0_SRCCTRL, 0x00003087);

	/* ----Read DPCD 0x00200-0x00204-------- */
	tc_write(DP0_AUXADDR, 0x00000200);
	tc_write(DP0_AUXCFG0, 0x00000409);

	tc_read(DP0_AUXSTATUS, &value);
	printk("Reg DP0_AUXSTATUS 0x%08x (should be 0x%08x)\n",
		value, 0x00000500);

	tc_read(DP0_AUXRDATA(0), &value);
	printk("Reg DP0_AUXRDATA0 0x%08x (should be 0x%08x)\n",
		value, 0x00770000);

	tc_read(DP0_AUXRDATA(1), &value);
	printk("Reg DP0_AUXRDATA1 0x%08x (should be 0x%08x)\n",
		value, 0x01);

	/* ----Enable ASSR on Panel------- */
	tc_write(DP0_AUXADDR, 0x00000102);
	tc_write(DP0_AUXWDATA(0), 0x00000020);
	tc_write(DP0_AUXCFG0, 0x00000108);

#if 0
	/* ----Enable EDID address to be mapped through DP link--- */
	tc_write(DP0_AUXI2CADR, 0x50 >> 1);
#endif

	return 0;
err:
	return ret;
}

static int tc_probe(struct device_d *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tc_data *tc;
	enum of_gpio_flags flags;
	u32 value;
	int ret;

	printf("TC358767 probe\n");

	tc = xzalloc(sizeof(struct tc_data));
	if (!tc)
		return -ENOMEM;

	tc->client = client;
	tc->dev = dev;

	tc->enable_gpio = of_get_named_gpio_flags(dev->device_node,
			"enable-gpios", 0, &flags);
	if (gpio_is_valid(tc->enable_gpio)) {
		if (!(flags & OF_GPIO_ACTIVE_LOW))
			tc->enable_active_high = 1;
	}

	tc->reset_gpio = of_get_named_gpio_flags(dev->device_node,
			"reset-gpios", 0, &flags);
	if (gpio_is_valid(tc->reset_gpio)) {
		if (!(flags & OF_GPIO_ACTIVE_LOW))
			tc->reset_active_high = 1;
	}

	ret = tc_read_reg(tc, TC_IDREG, &value);
	if (ret) {
		dev_err(tc->dev, "can not read device ID\n");
		goto err;
	}

	if (value != 0x6601) {
		dev_err(tc->dev, "invalid device ID: 0x%08x\n", value);
		ret = -EINVAL;
		goto err;
	}

	ret = tc_setup(tc);
	if (ret)
		goto err;

	/* register i2c aux port */
	tc->adapter.master_xfer = tc_aux_i2c_xfer;
	tc->adapter.nr = -1; /* any free */
	tc->adapter.dev.parent = dev;
	tc->adapter.dev.device_node = dev->device_node;
	/* Add I2C adapter */
	ret = i2c_add_numbered_adapter(&tc->adapter);
	if (ret < 0) {
		dev_err(tc->dev, "registration failed\n");
		goto err;
	}

	printf("TC358767 probed\n");

	return 0;

err:
	free(tc);
	return ret;
}
/*
static const struct platform_device_id tc_ids[] = {
	{ "toshiba,tc358767" },
	{  }
};
*/
static struct driver_d tc_driver = {
	.name		= "tc358767",
	.probe		= tc_probe,
	//.id_table	= tc_ids,
};

static int tc_init(void)
{
	return i2c_driver_register(&tc_driver);
}
device_initcall(tc_init);