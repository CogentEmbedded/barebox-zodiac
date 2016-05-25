/*
 *
 * tc358767 eDP encoder driver for barebox
 *
 * Copyright (C) 2016 CogentEmbedded Inc
 * Author: Andrey Gusakov <andrey.gusakov@cogentembedded.com>
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
#include <gpio.h>
#include <of_gpio.h>
#include <video/vpl.h>
#include <video/fourcc.h>
#include <asm-generic/div64.h>

/* gcd */
#include <linux/gcd.h>

/* hack */
#include "imx-ipu-v3/imx-ipu-v3.h"

/* debug */
#include <command.h>

/* registers */
#define DPIPXLFMT		0x0440

#define VPCTRL0			0x0450
#define HTIM01			0x0454
#define HTIM02			0x0458
#define VTIM01			0x045c
#define VTIM02			0x0460
#define VFUEN0			0x0464

#define TC_IDREG		0x0500
#define SYSSTAT			0x0508
#define SYSCTRL			0x0510

#define DP0CTL			0x0600
#define DP0_VIDMNGEN0		0x0610
#define DP0_VIDMNGEN1		0x0614
#define DP0_VMNGENSTATUS	0x0618
#define DP0_SECSAMPLE		0x0640
#define DP0_VIDSYNCDELAY	0x0644
#define DP0_TOTALVAL		0x0648
#define DP0_STARTVAL		0x064c
#define DP0_ACTIVEVAL		0x0650
#define DP0_SYNCVAL		0x0654
#define DP0_MISC		0x0658
#define DP0_AUXCFG0		0x0660
#define DP0_AUXCFG1		0x0664
#define DP0_AUXADDR		0x0668
#define DP0_AUXWDATA(i)		(0x066c + (i) * 4)
#define DP0_AUXRDATA(i)		(0x067c + (i) * 4)
#define DP0_AUXSTATUS		0x068c
#define DP0_AUXI2CADR		0x0698

#define DP0_SRCCTRL		0x06a0
#define DP0_SRCCTRL_SCRMBLDIS	(1 << 13)
#define DP0_SRCCTRL_EN810B	(1 << 12)
#define DP0_SRCCTRL_NOTP	(0 << 8)
#define DP0_SRCCTRL_TP1		(1 << 8)
#define DP0_SRCCTRL_TP2		(2 << 8)
#define DP0_SRCCTRL_LANESKEW	(1 << 7)
#define DP0_SRCCTRL_SSCG	(1 << 3)
#define DP0_SRCCTRL_LANES_1	(0 << 2)
#define DP0_SRCCTRL_LANES_2	(1 << 2)
#define DP0_SRCCTRL_BW27	(1 << 1)
#define DP0_SRCCTRL_BW162	(0 << 1)
#define DP0_SRCCTRL_AUTOCORRECT	(1 << 0)

#define DP0_LTSTAT		0x06d0
#define DP0_SNKLTCHGREQ		0x06d4
#define DP0_LTLOOPCTRL		0x06d8
#define DP0_SNKLTCTRL		0x06e4

#define DP_PHY_CTRL		0x0800
#define DP0_PLLCTRL		0x0900
#define DP1_PLLCTRL		0x0904	/* not defined in DS */
#define PXL_PLLCTRL		0x0908
#define PXL_PLLPARAM		0x0914
#define SYS_PLLPARAM		0x0918

#define TSTCTL			0x0a00

/* misc */
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

struct edp_link {
	u8 			rate;
	u8 			rev;
	u8			lanes;
	u8			enhanced;
	u8			assr;
	int			scrambler_dis;
	int			spread;
	int			coding8b10b;
	u8			swing;
	u8			preemp;
};

struct tc_data {
	struct i2c_client	*client;
	struct device_d		*dev;
	/* aux i2c */
	struct i2c_adapter	adapter;
	/* vlp */
	struct vpl vpl;

	/* link settings */
	struct edp_link		link;

	/* mode */
	struct fb_videomode	*mode;

	/* PLL pixelclock */
	u32			pll_clk;
	u32			pll_clk_real;

	int			test_pattern;

	u32			rev;
	u8			assr;

	char			*edid;

	int sd_gpio;
	int sd_active_high;
	int reset_gpio;
	int reset_active_high;
};
#define to_tc_i2c_struct(a)	container_of(a, struct tc_data, adapter)


struct tc_data *global_tc;

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
			/* Ack */
			return 0;
		case 0x40:
			/* Nack */
			return -EIO;
		case 0x80:
			dev_err(tc->dev, "i2c defer\n");
			return -EAGAIN;
		}
		return 0;
	}

	if (value & 0x02) {
		dev_err(tc->dev, "i2c access timeout!\n");
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

static int tc_aux_read(struct tc_data *tc, int reg, char *data, int size)
{
	int i = 0;
	int ret;
	u32 tmp;

	ret = tc_aux_i2c_wait_busy(tc, 100);
	if (ret)
		goto err;

	/* store address */
	tc_write(DP0_AUXADDR, reg);
	/* start transfer */
	tc_write(DP0_AUXCFG0, ((size - 1) << 8) | 0x09);

	ret = tc_aux_i2c_wait_busy(tc, 100);
	if (ret)
		goto err;

	ret = tc_aux_i2c_get_status(tc);
	if (ret)
		goto err;

	/* read data */
	while (i < size) {
		if ((i % 4) == 0)
			tc_read(DP0_AUXRDATA(i >> 2), &tmp);
		data[i] = tmp & 0xFF;
		tmp = tmp >> 8;
		i++;
	}

	return 0;
err:
	dev_err(tc->dev, "tc_aux_read error: %d\n", ret);
	return ret;
}

static int tc_aux_write(struct tc_data *tc, int reg, char *data, int size)
{
	int i = 0;
	int ret;
	u32 tmp = 0;

	ret = tc_aux_i2c_wait_busy(tc, 100);
	if (ret)
		goto err;

	i = 0;
	/* store data */
	while (i < size) {
		tmp = tmp | (data[i] << (8 * (i & 0x03)));
		i++;
		if (((i % 4) == 0) ||
		    (i == size)) {
			tc_write(DP0_AUXWDATA(i >> 2), tmp);
			tmp = 0;
		}
	}
	/* store address */
	tc_write(DP0_AUXADDR, reg);
	/* start transfer */
	tc_write(DP0_AUXCFG0, ((size - 1) << 8) | 0x08);

	ret = tc_aux_i2c_wait_busy(tc, 100);
	if (ret)
		goto err;

	ret = tc_aux_i2c_get_status(tc);
	if (ret)
		goto err;

	return 0;
err:
	dev_err(tc->dev, "tc_aux_write error: %d\n", ret);
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
	tc_write(DP0_AUXCFG0, ((msg->len - 1) << 8) | 0x01);

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

static int tc_aux_i2c_write(struct tc_data *tc, struct i2c_msg *msg)
{
	int i = 0;
	int ret;
	u32 tmp = 0;
	
	if (msg->flags & I2C_M_DATA_ONLY)
		return -EINVAL;

	if (msg->len > 16) {
		dev_err(tc->dev, "this bus support max 16 bytes per transfer\n");
		return -EINVAL;
	}

	ret = tc_aux_i2c_wait_busy(tc, 100);
	if (ret)
		goto err;

	/* store data */
	while (i < msg->len) {
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
	tc_write(DP0_AUXCFG0, ((msg->len - 1) << 8) | 0x00);

	ret = tc_aux_i2c_wait_busy(tc, 100);
	if (ret)
		goto err;

	ret = tc_aux_i2c_get_status(tc);
	if (ret)
		goto err;

	return 0;
err:
	return ret;
}

static int tc_aux_i2c_xfer(struct i2c_adapter *adapter,
			struct i2c_msg *msgs, int num)
{
	struct tc_data *tc = to_tc_i2c_struct(adapter);
	unsigned int i;
	int ret;

	/* check */
	for (i = 0; i < num; i++) {
		if (msgs[i].len > 16) {
			dev_err(tc->dev, "this bus support max 16 bytes per transfer\n");
			return -EINVAL;
		}
	}

	/* read/write data */
	for (i = 0; i < num; i++) {
		/* write/read data */
		if (msgs[i].flags & I2C_M_RD)
			ret = tc_aux_i2c_read(tc, &msgs[i]);
		else
			ret = tc_aux_i2c_write(tc, &msgs[i]);
		if (ret)
			goto err;
	}

err:
	return (ret < 0) ? ret : num;
}

static char *training_1_errors[] = {
	"No errors",
	"Aux write error",
	"Aux read error",
	"Max voltage reached error",
	"Loop counter expired error",
	"res", "res", "res"
};

static char *training_2_errors[] = {
	"No errors",
	"Aux write error",
	"Aux read error",
	"Clock recovery failed error",
	"Loop counter expired error",
	"res", "res", "res"
};

static u32 tc_srcctrl(struct tc_data *tc)
{
	u32 reg =
//		(tc->link.preemp << 28)| /* Pre-Emphasis for DisplayPort Port1 */
//		(tc->link.preemp << 20)| /* Pre-Emphasis for DisplayPort Port0 */
//		(tc->link.swing << 24) | /* Voltage Swing for DisplayPort Port1 */
//		(tc->link.swing << 16) | /* Voltage Swing for DisplayPort Port0 */
		DP0_SRCCTRL_NOTP |	/* no pattern */
		DP0_SRCCTRL_LANESKEW |	/* skew lane 1 data by two LSCLK cycles with respect to lane 0 data */
		(0 << 0) |		/* AutoCorrect Mode = 0 */
		0;

	if (tc->link.scrambler_dis)
		reg |= DP0_SRCCTRL_SCRMBLDIS;	/* Scrambler Disabled */
	if (tc->link.coding8b10b)
		reg |= DP0_SRCCTRL_EN810B;	/* Enable 8/10B Encoder (TxData[19:16] not used) */
	if (tc->link.spread)
		reg |= DP0_SRCCTRL_SSCG;	/* Spread Spectrum Enable */
	if (tc->link.lanes == 2)
		reg |= DP0_SRCCTRL_LANES_2;	/* Two Main Channel Lanes */
	if (tc->link.rate != 0x06)
		reg |= DP0_SRCCTRL_BW27;	/* 2.7 Gbps link */
	return reg;
}

static int tc_pxl_pll_en(struct tc_data *tc, u32 refclk)
{
	int ret;
	int i_pre, best_pre = 1;
	int i_post, best_post = 1;
	int div, best_div = 1;
	int mul, best_mul = 1;
	int delta, best_delta;
	int e_pre_div[] = {1, 2, 3, 5, 7};
	int e_post_div[] = {1, 2, 3, 5, 7};
	int best_pixelclock = 0;
	int vco_hi = 0;
	int pixelclock;

	pixelclock = tc->pll_clk;

	dev_info(tc->dev, "PLL: requested %d pixeclock, ref %d\n", pixelclock, refclk);
	best_delta = pixelclock;
	/* big loops */
	for (i_pre = 0; i_pre < ARRAY_SIZE(e_pre_div); i_pre++) {
		/* check restrictions */
		if ((refclk / e_pre_div[i_pre] > 200000000) ||
		    (refclk / e_pre_div[i_pre] < 1000000))
			continue;
		for (i_post = 0; i_post < ARRAY_SIZE(e_post_div); i_post++) {
			for (div = 1; div <= 16; div++) {
				u32 clk;
				u64 tmp;
				tmp = pixelclock * e_pre_div[i_pre] * e_post_div[i_post] * div;
				do_div(tmp, refclk);
				mul = tmp;

				clk = refclk / e_pre_div[i_pre] / div * mul / e_post_div[i_post];
				delta = clk - pixelclock;

				/* check limits */
				if ((mul < 1) ||
				    (mul > 128))
					continue;
				/* check restrictions */
				if ((refclk / e_pre_div[i_pre] / div * mul > 650000000) ||
				    (refclk / e_pre_div[i_pre] / div * mul < 150000000))
					continue;

				if (abs(delta) < abs(best_delta)) {
					best_pre = i_pre;
					best_post = i_post;
					best_div = div;
					best_mul = mul;
					best_delta = delta;
					best_pixelclock = clk;
				}
			}
		}
	}
	if (best_pixelclock == 0) {
		dev_err(tc->dev, "Failed to calc clock for %d pixelclock\n", pixelclock);
		return -EINVAL;
	}
	
	dev_info(tc->dev, "PLL: got %d, delta %d\n", best_pixelclock, best_delta);
	dev_info(tc->dev, "PLL: %d / %d / %d * %d / %d\n",
		refclk, e_pre_div[best_pre], best_div,
		best_mul, e_post_div[best_post]);

	/* if VCO >= 300 MHz */
	if (refclk / e_pre_div[best_pre] / best_div * best_mul >=
		300000000)
		vco_hi = 1;
	/* see DS */
	if (best_div == 16)
		best_div = 0;
	if (best_mul == 128)
		best_mul = 0;

	tc_write(PXL_PLLCTRL,
		(1 << 1) |	/* PLL bypass enable */
		(1 << 0) |	/* Enable PLL */
		0);
	
	tc_write(PXL_PLLPARAM,
		(vco_hi << 24)|			/* For PLL VCO >= 300 MHz = 1 */
		(e_pre_div[best_pre] << 20)|	/* External Pre-divider */
		(e_post_div[best_post] << 16)|	/* External Post-divider */
		(0x00 << 14)|			/* Use RefClk */
		(best_div << 8) |		/* Divider for PLL RefClk */
		(best_mul << 0) |		/* Multiplier for PLL */
		0);
	tc_write(PXL_PLLCTRL,
		(1 << 2) |	/* Force PLL parameter update register */
		(0 << 1) |	/* PLL bypass disabled */
		(1 << 0) |	/* Enable PLL */
		0);
	/* wait pll lock */
	mdelay(100);

	/* save */
	tc->pll_clk_real = best_pixelclock;

	return 0;
err:
	return ret;
}

static int tc_pxl_pll_dis(struct tc_data *tc)
{
	int ret;

	tc_write(PXL_PLLCTRL,
		(1 << 1) |	/* PLL bypass enable */
		(0 << 0) |	/* Disable PLL */
		0);

	return 0;
err:
	return ret;
}

static int tc_stream_clock_calc(struct tc_data *tc)
{
	int ret;
	/*
	 * If the Stream clock and Link Symbol clock are
	 * asynchronous with each other, the value of M changes over
	 * time. This way of generating link clock and stream
	 * clock is called Asynchronous Clock mode. The value M
	 * must change while the value N stays constant. The
	 * value of N in this Asynchronous Clock mode must be set
	 * to 2^15 or 32,768.
	 *
	 * LSCLK = 1/10 of high speed link clock
	 *
	 * f_STRMCLK = M/N * f_LSCLK
	 * M/N = f_STRMCLK / f_LSCLK
	 *
	 */
	tc_write(DP0_VIDMNGEN1, 32768);

	return 0;
err:
	return ret;
}

static int tc_test_pattern(struct tc_data *tc, unsigned int type)
{
	int ret;
	u32 value;

	if (type > 3)
		return -EINVAL;

	if (!tc->pll_clk)
		tc->pll_clk = PICOS2KHZ(tc->mode->pixclock) * 1000UL;

	if (type) {
		ret = tc_pxl_pll_en(tc, 19200000);
	} else {
		ret = tc_pxl_pll_dis(tc);
	}
	if (ret)
		goto err;

	/* set type */
	tc_read(TSTCTL, &value);
	value &= ~0x03;
	value |= type;
	tc_write(TSTCTL, value);

	/* set mode */
	tc_read(SYSCTRL, &value);
	value &= ~0x03;
	if (type)
		value |= 0x03;
	else
		value |= 0x02; /* DPI input */
	tc_write(SYSCTRL, value);

	return 0;
err:
	return ret;
}

static int tc_aux_link_setup(struct tc_data *tc)
{
	int ret;
	u32 value;
	int timeout;

	/* ----Setup DP-PHY / PLL------- */
	tc_write(SYS_PLLPARAM,
		(1 << 8) |	/* RefClk frequency is 19.2 MHz */
		(0 << 4) |	/* Use LSCLK based source */
		(1 << 0) |	/* LSCLK divider for SYSCLK = 2 */
		0);
	tc_write(DP_PHY_CTRL, //0x03000007);
		(1 << 25)|	/* AUX PHY BGR Enable */
		(1 << 24)|	/* PHY Power Switch Enable */
		(1 << 2) |	/* Reserved */
		(1 << 1) |	/* PHY Aux Channel0 Enable */
		0);
	tc_write(DP0_PLLCTRL,
		(1 << 2) |	/* Force PLL parameter update register */
		(0 << 1) |	/* PLL bypass disabled */
		(1 << 0) |	/* Enable PLL */
		0);
	/* wait PLL lock */
	mdelay(100);
	tc_write(DP1_PLLCTRL,
		(1 << 2) |	/* Force PLL parameter update register */
		(0 << 1) |	/* PLL bypass disabled */
		(1 << 0) |	/* Enable PLL */
		0);
	mdelay(100);

	timeout = 1000;
	do {
		tc_read(DP_PHY_CTRL, &value);
		udelay(1);
	} while ((!(value & (1 << 16))) && (--timeout));

	if (timeout == 0) {
		dev_err(tc->dev, "timeout waiting for phy become ready");
		return -ETIMEDOUT;
	}

	/* ----Setup AUX link */
	tc_write(DP0_AUXCFG1,
		(1 << 16) |	/* Aux Rx Filter Enable */
		(0x06 << 8) |	/* Aux Bit Period Calculator Threshhold */
		(0x3F << 0) |	/* Aux Response Timeout Timer */
		0);

	return 0;
err:
	dev_err(tc->dev, "tc_aux_link_setup failed: %d\n", ret);
	return ret;
}

static int tc_get_display_props(struct tc_data *tc)
{
	int ret;
	/* temp buffer */
	u8 tmp[16];

	/* ----Read DP Rx Link Capability-------- */
	ret = tc_aux_read(tc, 0x000000, tmp, 8);
	if (ret)
		goto err_dpcd_read;
	/* check rev 1.0 or 1.1 */
	if ((tmp[1] != 0x06) && (tmp[1] != 0x0a))
		goto err_dpcd_inval;

	tc->assr = !(tc->rev & 0x02);
	tc->link.rev = tmp[0];
	tc->link.rate = tmp[1];
	tc->link.lanes = tmp[2] & 0x0f;
	tc->link.enhanced = !!(tmp[2] & 0x80);
	tc->link.spread = tmp[3] & 0x01;
	tc->link.coding8b10b = tmp[6] & 0x01;
	tc->link.scrambler_dis = 0;
	/* read assr */
	ret = tc_aux_read(tc, 0x00010a, tmp, 1);
	if (ret)
		goto err_dpcd_read;
	tc->link.assr = tmp[0] & 0x01;

	dev_notice(tc->dev, "DPCD rev: %d.%d, rate: %s, lanes: %d, framing: %s\n",
		tc->link.rev >> 4,
		tc->link.rev & 0x0f,
		(tc->link.rate == 0x06) ? "1.62Gbps" : "2.7Gbps",
		tc->link.lanes,
		tc->link.enhanced ? "enhanced" : "non-enhanced");
	dev_notice(tc->dev, "ANSI 8B/10B: %d\n", tc->link.coding8b10b);
	dev_notice(tc->dev, "Display ASSR: %d, TC358767 ASSR: %d\n",
		tc->link.assr, tc->assr);

	return 0;

err_dpcd_read:
	dev_err(tc->dev, "failed to read DPCD: %d\n", ret);
	return ret;
err_dpcd_inval:
	dev_err(tc->dev, "invalid DPCD\n");
	return -EINVAL;
}

static int tc_set_video_mode(struct tc_data *tc, struct fb_videomode *mode)
{
	int ret;
	int htotal;
	int vtotal;
	int vid_sync_dly;

	htotal = mode->hsync_len + mode->left_margin + mode->xres +
		mode->right_margin;
	vtotal = mode->vsync_len + mode->upper_margin + mode->yres +
		mode->lower_margin;

	dev_info(tc->dev, "set mode %dx%d\n", mode->xres, mode->yres);
	dev_info(tc->dev, "H margin %d,%d sync %d\n",
		mode->left_margin, mode->right_margin, mode->hsync_len);
	dev_info(tc->dev, "V margin %d,%d sync %d\n",
		mode->upper_margin, mode->lower_margin, mode->vsync_len);
	dev_info(tc->dev, "total: %dx%d\n", htotal, vtotal);


	/* LCD Ctl Frame Size */
	tc_write(VPCTRL0,
		(0x40 << 20)|	/* VSDELAY */
		(1 << 8) |	/* RGB888 */
		(0 << 4) |	/* timing gen is disabled */
		(0 << 0) |	/* Magic Square is disabled */
		0);
	tc_write(HTIM01,
		(mode->left_margin << 16) |	/* H back porch */
		(mode->hsync_len << 0));	/* Hsync */
	tc_write(HTIM02,
		(mode->right_margin << 16) |	/* H front porch */
		(mode->xres << 0));		/* width */
	tc_write(VTIM01,
		(mode->upper_margin << 16) |	/* V back porch */
		(mode->vsync_len << 0));	/* Vsync */
	tc_write(VTIM02,
		(mode->lower_margin << 16) |	/* V front porch */
		(mode->yres << 0));		/* height */
	tc_write(VFUEN0, 0x00000001);		/* update settings */

	/* Teast pattern settings */
	tc_write(TSTCTL,
		(120 << 24) |	/* Red Color component value */
		(20 << 16) |	/* Green Color component value */
		(99 << 8) |	/* Blue Color component value */
		(1 << 4) |	/* Enable I2C Filter */
		(2 << 0) |	/* Color bar Mode */
		0);

	/* DP Main Stream Attirbutes */
	vid_sync_dly = mode->hsync_len + mode->left_margin + mode->xres;
	tc_write(DP0_VIDSYNCDELAY,
		(0x003e << 16) | 	/* thresh_dly */
		(vid_sync_dly << 0)|	/* vid_sync_dly */
		0);

	tc_write(DP0_TOTALVAL, (vtotal << 16) | (htotal));

	tc_write(DP0_STARTVAL,
		((mode->upper_margin + mode->vsync_len) << 16) |
		((mode->left_margin + mode->hsync_len) << 0));

	tc_write(DP0_ACTIVEVAL, (mode->yres << 16) | (mode->xres));

	tc_write(DP0_SYNCVAL,
		//(1 << 31)|			/* Vsync active low */
		(mode->vsync_len << 16)|	/* Vsync width */
		//(1 << 15)|			/* Hsync active low */
		(mode->hsync_len << 0)|		/* Hsync width */
		0);

	tc_write(DPIPXLFMT,
		(1 << 10)|			/* VSYNC Polarity Control = Low */
		(1 << 9) |			/* HSYNC Polarity Control = Low */
		(0 << 8) |			/* DE Polarity Control */
		(0 << 2) |			/* Alignment of pixels = 0: Config1 (LSB aligned) */
		(0 << 0) |			/* RGB888 */
		0);

	tc_write(DP0_MISC,
		(0x3e << 23)|			/* max_tu_symbol */
		(0x3f << 16)|			/* tu_size */
		(1 << 5) |			/* 8 bit per component */
		0);

	return 0;
err:
	return ret;
}

static int tc_main_link_setup(struct tc_data *tc)
{
	int ret;
	u32 value;
	/* temp buffer */
	u8 tmp[16];
	int timeout;
	int retry;

	/* display mode should be set at this point */
	if (!tc->mode)
		return -EINVAL;

	/* from exel file - DP0_SrcCtrl */
	tc_write(0x06A0, 0x00003087);
	/* from exel file - DP1_SrcCtrl */
	tc_write(0x07a0, 0x00003083);
	tc_write(SYS_PLLPARAM, //0x00000101);
		(1 << 8) |	/* RefClk frequency is 19.2 MHz */
		(0 << 4) |	/* Use LSCLK based source */
		(1 << 0) |	/* LSCLK divider for SYSCLK = 2 */
		0);
	/* ----Setup Main Link-------- */
	tc_write(DP_PHY_CTRL, //0x03000007);	should be 0x03000017
		(1 << 25)|	/* AUX PHY BGR Enable */
		(1 << 24)|	/* PHY Power Switch Enable */
		(1 << 2) |	/* Reserved */
		(1 << 1) |	/* PHY Aux Channel0 Enable */
		(1 << 0) |	/* PHY Main Channel0 Enable */
		0);
	mdelay(100);

	/* PLL setup */
	tc_write(DP0_PLLCTRL, //0x00000005);
		(1 << 2) |	/* Force PLL parameter update register */
		(0 << 1) |	/* PLL bypass disabled */
		(1 << 0) |	/* Enable PLL */
		0);
	/* wait PLL lock */
	mdelay(100);
	tc_write(DP1_PLLCTRL, //0x00000005);	//???
		(1 << 2) |	/* Force PLL parameter update register */
		(0 << 1) |	/* PLL bypass disabled */
		(1 << 0) |	/* Enable PLL */
		0);
	/* wait PLL lock */
	mdelay(100);
	/* PXL PLL setup */
	if (tc->test_pattern) {
		ret = tc_pxl_pll_en(tc, 19200000);
		if (ret)
			goto err;
	}

	/* ----Reset/Enable Main Links-------- */
	tc_write(DP_PHY_CTRL,
		(1 << 28)|	/* DP PHY Global Soft Reset */
		(1 << 25)|	/* AUX PHY BGR Enable */
		(1 << 24)|	/* PHY Power Switch Enable */
		(1 << 12)|	/* Reset DP PHY1 Main Channel */
		(1 << 8) |	/* Reset DP PHY0 Main Channel */
		(1 << 2) |	/* Reserved */
		(1 << 1) |	/* PHY Aux Channel0 Enable */
		(1 << 0) |	/* PHY Main Channel0 Enable */
		0);
	udelay(100);
	tc_write(DP_PHY_CTRL,
		(1 << 25)|	/* AUX PHY BGR Enable */
		(1 << 24)|	/* PHY Power Switch Enable */
		(1 << 2) |	/* Reserved */
		(1 << 1) |	/* PHY Aux Channel0 Enable */
		(1 << 0) |	/* PHY Main Channel0 Enable */
		0);

	timeout = 1000;
	do {
		tc_read(DP_PHY_CTRL, &value);
		udelay(1);
	} while ((!(value & (1 << 16))) && (--timeout));

	if (timeout == 0) {
		dev_err(tc->dev, "timeout waiting for phy become ready");
		return -ETIMEDOUT;
	}

	/* ----Set misc---- */
	/* 8 bits per color */
	tc_read(DP0_MISC, &value);
	value = value | (1 << 5);
	tc_write(DP0_MISC, value);

	/*
	 * ASSR mode
	 * on TC358767 side ASSR configured through strap pin
	 * seems there is no way to change this setting from SW
	 *
	 * check is tc configured for same mode
	 */
	if (tc->assr != tc->link.assr) {
		dev_info(tc->dev, "Trying to set display to ASSR: %d\n",
			tc->assr);
		/* try to set ASSR on display side */
		tmp[0] = tc->assr;
		ret = tc_aux_write(tc, 0x00010a, tmp, 1);
		if (ret)
			goto err_dpcd_read;
		/* read back */
		ret = tc_aux_read(tc, 0x00010a, tmp, 1);
		if (ret)
			goto err_dpcd_read;

		if (tmp[0] != tc->assr) {
			dev_err(tc->dev, "failed to switch display ASSR to %d,"
					 " falling to unscrambled mode\n",
				tc->assr);
			/* trying with disabled scrambler */
			tc->link.scrambler_dis = 1;
		}
	}

	/* ----Setup Link & DPRx Config for Training-------- */
	/* LINK_BW_SET */
	tmp[0] = tc->link.rate;
	/* LANE_COUNT_SET */
	tmp[1] = tc->link.lanes;
	if (tc->link.enhanced)
		tmp[1] |= (1 << 7);
	ret = tc_aux_write(tc, 0x000100, tmp, 2);
	if (ret)
		goto err_dpcd_write;

	/* DOWNSPREAD_CTRL */
	if (tc->link.spread)
		tmp[0] = 0x10;	/* down-spreading enable */
	else
		tmp[0] = 0x00;
	/* MAIN_LINK_CHANNEL_CODING_SET */
	if (tc->link.coding8b10b)
		tmp[1] = 0x01;	/* 8B10B */
	else
		tmp[1] = 0x00;
	ret = tc_aux_write(tc, 0x000107, tmp, 2);
	if (ret)
		goto err_dpcd_write;

	/* ----Set DPCD 00102h for Training Pat 1-------- */
	tc_write(DP0_SNKLTCTRL, 0x00000021);
	tc_write(DP0_LTLOOPCTRL,
		(0x0f << 28)|	/* Defer Iteration Count */
		(0x0f << 24)|	/* Loop Iteration Count */
		(0x0d << 0)|	/* Loop Timer Delay */
		0);

	retry = 5;
	do {
		/* ----Set DP0 Trainin Pattern 1-------- */
		tc_write(DP0_SRCCTRL, tc_srcctrl(tc) |
			DP0_SRCCTRL_SCRMBLDIS |		/* Scrambler Disabled */
			DP0_SRCCTRL_TP1 |		/* Training Pattern 1 */
			DP0_SRCCTRL_LANESKEW |		/* skew lane 1 data by two LSCLK cycles with respect to lane 0 data */
			DP0_SRCCTRL_AUTOCORRECT |	/* AutoCorrect Mode = 1 */
			0);

		/* ----Enable DP0 to start Link Training-------- */
		tc_write(DP0CTL, 0x00000001);

		/* wait */
		timeout = 1000;
		do {
			tc_read(DP0_LTSTAT, &value);
			udelay(1);
		} while ((!(value & (1 << 13))) && (--timeout));
		if (timeout == 0) {
			dev_err(tc->dev, "training timeout!\n");
		} else {
			dev_info(tc->dev, "Link training phase %d done after %d uS: %s\n",
				(value >> 11) & 0x03, 1000 - timeout,
				training_1_errors[(value >> 8) & 0x07]);
			if (((value >> 8) & 0x07) == 0)
				break;
		}
		/* restart */
		tc_write(DP0CTL, 0x00000000);
		udelay(10);
	} while (--retry);
	if (retry == 0)
		dev_err(tc->dev, "failed to finish training pohase 1\n");

	/* ----Set DPCD 00102h for Link Traing Part 2-------- */
	tc_write(DP0_SNKLTCTRL, 0x00000022);

	retry = 5;
	do {
		/* ----Set DP0 Trainin Pattern 2-------- */
		tc_write(DP0_SRCCTRL, tc_srcctrl(tc) |
			DP0_SRCCTRL_SCRMBLDIS |		/* Scrambler Disabled */
			DP0_SRCCTRL_TP2 |		/* Training Pattern 2 */
			DP0_SRCCTRL_LANESKEW |		/* skew lane 1 data by two LSCLK cycles with respect to lane 0 data */
			DP0_SRCCTRL_AUTOCORRECT |	/* AutoCorrect Mode = 1 */
			0);

		/* remove this after finishing debug */
		/* ----Enable DP0 to start Link Training-------- */
		tc_write(DP0CTL, 0x00000001);

		/* wait */
		timeout = 1000;
		do {
			tc_read(DP0_LTSTAT, &value);
			udelay(1);
		} while ((!(value & (1 << 13))) && (--timeout));
		if (timeout == 0) {
			dev_err(tc->dev, "training timeout!\n");
		} else {
			dev_info(tc->dev, "Link training phase %d done after %d uS: %s\n",
				(value >> 11) & 0x03, 1000 - timeout,
				training_2_errors[(value >> 8) & 0x07]);
			/* in case of two lanes */
			if (((value & 0x7f) == 0x7f) && (tc->link.lanes == 2))
				break;
			/* in case of one line */
			if (((value & 0x7f) == 0x0f) && (tc->link.lanes == 1))
				break;
		}
		/* restart */
		tc_write(DP0CTL, 0x00000000);
		udelay(10);
	} while (--retry);
	if (retry == 0)
		dev_err(tc->dev, "failed to finish training pohase 2\n");

	/* ----Clear DPCD 00102h-------- */
	/* Note: Can Not use DP0_SNKLTCTRL (0x06E4) short cut */
	tmp[0] = 0x00;
	if (tc->link.scrambler_dis)
		tmp[0] |= 0x20;
	ret = tc_aux_write(tc, 0x000102, tmp, 1); /* TRAINING_PATTERN_SET */
	if (ret)
		goto err_dpcd_write;

	tc_write(DP0_SRCCTRL, tc_srcctrl(tc) |
		DP0_SRCCTRL_LANESKEW |		/* skew lane 1 data by two LSCLK cycles with respect to lane 0 data */
		DP0_SRCCTRL_AUTOCORRECT |	/* Need? AutoCorrect Mode = 1 */
		0);

	/* wait */
	timeout = 100;
	do {
		udelay(1);
		/* ----Read DPCD 0x00200-0x00206-------- */
		ret = tc_aux_read(tc, 0x000200, tmp, 7);
		if (ret)
			goto err_dpcd_read;
	} while ((--timeout) &&
		((tmp[2] != 0x77) ||
		 ((tmp[4] & 0x01) != 0x01)/* ||
		 ((tmp[5] & 0x01) != 0x01)*/));

	if (timeout == 0) {
		dev_info(tc->dev, "0x0200 SINK_COUNT: 0x%02x\n", tmp[0]);
		dev_info(tc->dev, "0x0201 DEVICE_SERVICE_IRQ_VECTOR: 0x%02x\n", tmp[1]);
		dev_info(tc->dev, "0x0202 LANE0_1_STATUS: 0x%02x\n", tmp[2]);
		dev_info(tc->dev, "0x0204 LANE_ALIGN__STATUS_UPDATED: 0x%02x\n", tmp[4]);
		dev_info(tc->dev, "0x0205 SINK_STATUS: 0x%02x\n", tmp[5]);
		dev_info(tc->dev, "0x0206 ADJUST_REQUEST_LANE0_1: 0x%02x\n", tmp[6]);

		if (tmp[2] != 0x77)
			dev_err(tc->dev, "Lane0/1 not ready\n");
		if ((tmp[4] & 0x01) != 0x01)
			dev_err(tc->dev, "Lane0/1 not aligned\n");
		/*
		if ((tmp[5] & 0x01) != 0x01)
			dev_err(tc->dev, "Sink not ready\n");
		*/
		return -EAGAIN;
	}

	/* set video mode */
	ret = tc_set_video_mode(tc, tc->mode);
	if (ret)
		goto err;

	/* set M/N */
	ret = tc_stream_clock_calc(tc);
	if (ret)
		goto err;

	return 0;
err:
	return ret;
err_dpcd_read:
	dev_err(tc->dev, "failed to read DPCD: %d\n", ret);
	return ret;
err_dpcd_write:
	dev_err(tc->dev, "failed to write DPCD: %d\n", ret);
	return ret;
}


static int tc_main_link_stream(struct tc_data *tc, int state)
{
	int ret;
	u32 value;

	dev_info(tc->dev, "stream: %d\n", state);

	if (state) {
		value =
			(1 << 6) |	/* enable the auto-generation of M/N values for video */
			(1 << 0) |	/* DPTX function enable */
			0;

		if (tc->link.enhanced)
			value |= (1 << 5);	/* Enhanced Framing Enable */
		tc_write(DP0CTL, value);
		/*
		 * Note that vid_en_i or vid_en_i assertion should be delayed by atleast
		 * vid_n_i x LSCLK cycles from the time vid_mn_gen_i is asserted in
		 * order to generate stable values for vid_m_i.
		 */
		mdelay(100);
		value |= (1 << 1);		/* Video transmission enable */
		tc_write(DP0CTL, value);
		/* Set input interface */
		if (tc->test_pattern)
			value = (3 << 4) | (3 << 0);
		else
			value = (2 << 4) | (2 << 0);
		tc_write(SYSCTRL, value);
	} else {
		tc_write(DP0CTL, 0x00000000);
	}

	return 0;
err:
	return ret;
}

static int tc_get_videomodes(struct tc_data *tc, struct display_timings *timings)
{
	int ret;
#if 0
	/*
	 * does not work due to limitation of eDP i2c
	 */
	timings->edid = edid_read_i2c(&tc->adapter);
	if (!timings->edid)
		return -EINVAL;

	return edid_to_display_timings(timings, timings->edid);
#else
	if (tc->edid) {
		struct fb_videomode *mode;

		ret = edid_to_display_timings(timings, tc->edid);

		mode = timings->modes;
		/* hsync, vsync active low */
		mode->sync &= ~(FB_SYNC_HOR_HIGH_ACT |
				FB_SYNC_VERT_HIGH_ACT);
	} else {
		struct fb_videomode *mode;

		dev_info(tc->dev, "no EDID, assume Innolux N133HSE\n");

		/* one mode */
		mode = xzalloc(1 * sizeof(struct fb_videomode));
		timings->num_modes = 1;
		timings->modes = mode;

		/* fill */
		mode->name = "Innolux N133HSE";
		mode->refresh = 60;
		mode->xres = 1920;
		mode->yres = 1080;
		mode->pixclock = KHZ2PICOS(138500);
		mode->left_margin = 80;
		mode->right_margin = 40;
		mode->upper_margin = 16;
		mode->lower_margin = 14;
		mode->hsync_len = 40;
		mode->vsync_len = 2;
		mode->sync = 0;
		mode->vmode = 0;
		mode->display_flags = 0;

		ret = 0;
	}
#endif

	return ret;
}

#define ARRRAY_SIZE(a)	(sizeof(a) / arraysize(a[0]))
static int tc_debug_dump(struct tc_data *tc)
{
	int i;
	int ret;
	u8 tmp[16];
	struct {
		int	addr;
		int	size;
		char	*name;
	} regs_dpcd[] = {
		{0x006, 1, "MAIN_LINK_CHANNEL_CODING"},
		{0x100, 1, "LINK_BW_SET"},
		{0x101, 1, "LANE_COUNT_SET"},
		{0x102, 1, "TRAINING_PATTERN_SET"},
		{0x103, 1, "TRAINING_LANE0_SET"},
		{0x107, 1, "DOWNSPREAD_CTRL"},
		{0x108, 1, "MAIN_LINK_CHANNEL_CODING_SET"},
		{0x10A, 1, "ASSR???"},
		{0x200, 1, "SINK_COUNT"},
		{0x202, 1, "LANE0_1_STATUS"},
		{0x204, 1, "LANE_ALIGN__STATUS_UPDATED"},
		{0x205, 1, "SINK_STATUS"},
		{0x206, 1, "ADJUST_REQUEST_LANE0_1"},
		/* optional */
		{0x210, 2, "SYMBOL_ERROR_COUNT_LANE0"},
		{0x212, 2, "SYMBOL_ERROR_COUNT_LANE1"},
		{0x600, 1, "SET_POWER"},
	};

	struct {
		int	addr;
		char	*name;
	} regs_tc[] ={
		{DP0CTL, "DP0CTL"},
		{DP0_SRCCTRL, "DP0_SRCCTRL"},
		{SYSSTAT, "SYSSTAT"},
		{SYSCTRL, "SYSCTRL"},
		{DPIPXLFMT, "DPIPXLFMT"},
		{DP0_VIDMNGEN0, "DP0_VIDMNGEN0"},
		{DP0_VIDMNGEN1, "DP0_VIDMNGEN1"},
		{DP0_VMNGENSTATUS, "DP0_VMNGENSTATUS"},
		{DP0_SNKLTCHGREQ, "DP0_SNKLTCHGREQ"},
	};

	printf("tc358767 regs:\n");
	for (i = 0; i < ARRAY_SIZE(regs_tc); i++) {
		u32 value;

		tc_read(regs_tc[i].addr, &value);
		printf("0x%06x %s: 0x%08x\n",
			regs_tc[i].addr,
			regs_tc[i].name ? regs_tc[i].name : "",
			value);
	}

	printf("DPCD regs:\n");
	for (i = 0; i < ARRAY_SIZE(regs_dpcd); i++) {
		if (regs_dpcd[i].size) {
			ret = tc_aux_read(tc, regs_dpcd[i].addr, tmp, regs_dpcd[i].size);
			if (ret)
				goto err;
		}
		if (regs_dpcd[i].size == 0)
			printf("%s\n",
				regs_dpcd[i].name ? regs_dpcd[i].name : "");
		else if (regs_dpcd[i].size == 1)
			printf("0x%04x %s: 0x%02x\n",
				regs_dpcd[i].addr,
				regs_dpcd[i].name ? regs_dpcd[i].name : "",
				tmp[0]);
		else if (regs_dpcd[i].size == 2)
			printf("0x%04x %s: 0x%04x\n",
				regs_dpcd[i].addr,
				regs_dpcd[i].name ? regs_dpcd[i].name : "",
				(tmp[1] << 8) | tmp[0] );
			
	}

	if (1) {
		u32 rate;
		u32 M;
		u32 N;
		u64 freq;

		printf("Recover freq:\n");
		tc_read(DP0_VIDMNGEN1, &N);
		tc_read(DP0_VMNGENSTATUS, &M);

		if (tc->link.rate != 0x06)
			rate = 270;
		else
			rate = 162;

		freq = (u64)rate * (u64)M * 1000000;
		do_div(freq, N);
		printf("%dM * %d / %d = %u\n",
			rate, M, N, (u32)freq);
		if (tc->test_pattern)
			printf("Should be %u\n", tc->pll_clk_real);
	}

	return 0;
err:
	return ret;
}

#define DDC_BLOCK_READ		5
#define DDC_SEGMENT_ADDR	0x30
#define DDC_ADDR		0x50
#define EDID_LENGTH		0x80
static int tc_read_edid(struct tc_data *tc)
{
	int i = 0;
	int ret;
	int block;
	unsigned char start = 0;
	unsigned char segment = 0;

	struct i2c_msg msgs[] = {
		{
			.addr	= DDC_SEGMENT_ADDR,
			.flags	= 0,
			.len	= 1,
			.buf	= &segment,
		}, {
			.addr	= DDC_ADDR,
			.flags	= 0,
			.len	= 1,
			.buf	= &start,
		}, {
			.addr	= DDC_ADDR,
			.flags	= I2C_M_RD,
		}
	};
	tc->edid = xmalloc(EDID_LENGTH);

	do {
		block = MIN(DDC_BLOCK_READ, EDID_LENGTH - i);

		msgs[2].buf = tc->edid + i;
		msgs[2].len = block;

		ret = i2c_transfer(&tc->adapter, msgs, 3);
		if (ret < 0)
			goto err;

		i += DCC_BLOCK_READ;
		start = i;
	} while(i < EDID_LENGTH);

#if 0
	printf("eDP display EDID:\n");
	for (i = 0; i < EDID_LENGTH; i++) {
		if ((i) && ((i % 16) == 0))
			printf("\n");
		printf("%02x ", tc->edid[i]);
	}
	printf("\n");
#endif

	return 0;
err:
	free(tc->edid);
	tc->edid = NULL;
	dev_err(tc->dev, "tc_read_edid failed: %d\n", ret);
	return ret;
}

static int tc_ioctl(struct vpl *vpl, unsigned int port,
			unsigned int cmd, void *ptr)
{
	struct tc_data *tc = container_of(vpl,
			struct tc_data, vpl);
	/* hack */
	struct ipu_di_mode *ipu_mode;
	int ret = 0;

	switch (cmd) {
	case VPL_PREPARE:
		dev_dbg(tc->dev, "VPL_PREPARE\n");
		tc->mode = ptr;
	break;

	case VPL_ENABLE:
		dev_dbg(tc->dev, "VPL_ENABLE\n");
		ret = tc_main_link_setup(tc);
		if (ret < 0)
			break;

		ret = tc_main_link_stream(tc, 1);
	break;

	case VPL_DISABLE:
		dev_dbg(tc->dev, "VPL_DISABLE\n");

		ret = tc_main_link_stream(tc, 0);
		if (ret < 0)
			break;

	break;

	case VPL_GET_VIDEOMODES:
		dev_dbg(tc->dev, "VPL_GET_VIDEOMODES\n");

		ret = tc_get_videomodes(tc, ptr);
	break;

	/* hack */
	case IMX_IPU_VPL_DI_MODE:
		dev_dbg(tc->dev, "IMX_IPU_VPL_DI_MODE\n");

		ipu_mode = ptr;

		ipu_mode->di_clkflags = IPU_DI_CLKMODE_NON_FRACTIONAL;
		ipu_mode->interface_pix_fmt = V4L2_PIX_FMT_RGB24;
	break;
		
	default:
		break;
	}

	return ret;
}


static int do_edp_debug(int argc, char *argv[])
{
	int ret;
	struct tc_data *tc = global_tc;

	if (!tc)
		return -ENODEV;

	if (argc == 1) {
		tc_debug_dump(tc);
		return 0;
	}

	if ((argc == 2) && (!strcmp(argv[1], "s"))) {
		tc_main_link_setup(tc);
		tc_main_link_stream(tc, 1);
		tc_debug_dump(tc);
		return 0;
	}

	/* Shut Down */
	if ((argc == 3) && (!strcmp(argv[1], "sd"))) {
		int value;
		value = simple_strtol(argv[2], NULL, 0);
		
		gpio_direction_output(tc->sd_gpio, value);

		return 0;
	}

	if ((argc == 3) && (!strcmp(argv[1], "g"))) {
		/* get DPCD register */
		int reg;
		u8 value;

		reg = simple_strtol(argv[2], NULL, 0);

		ret = tc_aux_read(tc, reg, &value, 1);
		if (ret)
			goto err;
		printf("0x%06x: 0x%02x\n", reg, value);
		return 0;
	}

	if ((argc == 4) && (!strcmp(argv[1], "w"))) {
		/* write DPCD register */
		int reg;
		u8 value;

		reg = simple_strtol(argv[2], NULL, 0);
		value = simple_strtol(argv[3], NULL, 0);

		ret = tc_aux_write(tc, reg, &value, 1);
		if (ret)
			goto err;
		return 0;
	}

	/* PXL_PLL used for DSI interface and test pattern generator */
	if ((argc == 3) && (!strcmp(argv[1], "p"))) {
		tc->pll_clk = simple_strtol(argv[2], NULL, 0);

		ret = tc_pxl_pll_en(tc, 19200000);
		if (ret)
			goto err;
		return 0;
	}

	if ((argc == 3) && (!strcmp(argv[1], "t"))) {
		int type;

		type = simple_strtol(argv[2], NULL, 0);
		tc->test_pattern = type;
		ret = tc_test_pattern(tc, type);
		if (ret)
			goto err;
		return 0;
	}

	return COMMAND_ERROR_USAGE;
err:
	return ret;
}


BAREBOX_CMD_HELP_START(edp_debug)
BAREBOX_CMD_HELP_TEXT("eDP debug tools")
BAREBOX_CMD_HELP_OPT("no args\t", "dump some regs")
BAREBOX_CMD_HELP_OPT("s\t\t", "retry setup")
BAREBOX_CMD_HELP_OPT("g addr\t\t", "get DPCD reg")
BAREBOX_CMD_HELP_OPT("w addr value\t", "set DPCD reg")
BAREBOX_CMD_HELP_OPT("p value\t", "set PXL_PLL clock")
BAREBOX_CMD_HELP_OPT("t value\t", "enable test pattern (1-3), 0-disable")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(edp_debug)
	.cmd		= do_edp_debug,
	BAREBOX_CMD_DESC("eDP registers dump")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_edp_debug_help)
BAREBOX_CMD_END

static int tc_probe(struct device_d *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tc_data *tc;
	enum of_gpio_flags flags;
	int ret;

	tc = xzalloc(sizeof(struct tc_data));
	if (!tc)
		return -ENOMEM;

	tc->client = client;
	tc->dev = dev;

	tc->sd_gpio = of_get_named_gpio_flags(dev->device_node,
			"sd-gpios", 0, &flags);
	if (gpio_is_valid(tc->sd_gpio)) {
		if (!(flags & OF_GPIO_ACTIVE_LOW))
			tc->sd_active_high = 1;
	}

	tc->reset_gpio = of_get_named_gpio_flags(dev->device_node,
			"reset-gpios", 0, &flags);
	if (gpio_is_valid(tc->reset_gpio)) {
		if (!(flags & OF_GPIO_ACTIVE_LOW))
			tc->reset_active_high = 1;
	}

	if (gpio_is_valid(tc->sd_gpio)) {
		ret = gpio_request(tc->sd_gpio, "tc358767");
		if (ret) {
			dev_err(tc->dev, "SD (%d) can not be requested\n", tc->sd_gpio);
			return ret;
		}
		gpio_direction_output(tc->sd_gpio, 0);
	}

	ret = tc_read_reg(tc, TC_IDREG, &tc->rev);
	if (ret) {
		dev_err(tc->dev, "can not read device ID\n");
		goto err;
	}

	if ((tc->rev != 0x6601) && (tc->rev != 0x6603)) {
		dev_err(tc->dev, "invalid device ID: 0x%08x\n", tc->rev);
		ret = -EINVAL;
		goto err;
	}

	ret = tc_aux_link_setup(tc);
	if (ret)
		goto err;

	ret = tc_get_display_props(tc);
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

	/* add vlp */
	tc->vpl.node = dev->device_node;
	tc->vpl.ioctl = tc_ioctl;
	ret = vpl_register(&tc->vpl);
	if (ret)
		return ret;

	/* pre-read edid */
	ret = tc_read_edid(tc);
	/* ignore it for now */
	if (ret)
		dev_err(tc->dev, "EDID read error\n");

	global_tc = tc;

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