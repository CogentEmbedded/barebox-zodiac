/*
 * Copyright (C) 2015 Nikita Yushchenko, CogentEmbedded, Inc
 * Copyright (C) 2015 Andrey Gusakov, CogentEmbedded, Inc
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

#define pr_fmt(fmt) "zii-rdu1: " fmt

#include <common.h>
#include <init.h>
#include <of.h>
#include <mach/bbu.h>
#include <notifier.h>
#include <spi/spi.h>
#include <mfd/mc13xxx.h>
#include <mach/imx5.h>
#include <mach/spi.h>
#include <mach/generic.h>
#include <mach/iomux-mx51.h>
#include <mach/devices-imx51.h>
#include <mach/revision.h>
#include <libfile.h>
#include <param.h>
#include <unistd.h>
#include <fcntl.h>

static void zii_rdu1_power_init(struct mc13xxx *mc13xxx)
{
	u32 val;

	/* Write needed to Power Gate 2 register */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_POWER_MISC, &val);
	val &= ~0x10000;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_POWER_MISC, val);

	/* Write needed to update Charger 0 */
	mc13xxx_reg_write(mc13xxx, MC13892_REG_CHARGE, 0x0023807F);

	/* power up the system first */
	mc13xxx_reg_write(mc13xxx, MC13892_REG_POWER_MISC, 0x00200000);

	if (imx_silicon_revision() < IMX_CHIP_REV_3_0) {
		/* Set core voltage to 1.1V */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_0, &val);
		val &= ~0x1f;
		val |= 0x14;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_0, val);

		/* Setup VCC (SW2) to 1.25 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_1, &val);
		val &= ~0x1f;
		val |= 0x1a;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_1, val);

		/* Setup 1V2_DIG1 (SW3) to 1.25 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_2, &val);
		val &= ~0x1f;
		val |= 0x1a;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_2, val);
	} else {
		/* Setup VCC (SW2) to 1.225 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_1, &val);
		val &= ~0x1f;
		val |= 0x19;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_1, val);

		/* Setup 1V2_DIG1 (SW3) to 1.2 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_2, &val);
		val &= ~0x1f;
		val |= 0x18;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_2, val);
	}

	if (mc13xxx_revision(mc13xxx) < MC13892_REVISION_2_0) {
		/* Set switchers in PWM mode for Atlas 2.0 and lower */
		/* Setup the switcher mode for SW1 & SW2*/
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_4, &val);
		val &= ~0x3c0f;
		val |= 0x1405;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_4, val);

		/* Setup the switcher mode for SW3 & SW4 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_5, &val);
		val &= ~0xf0f;
		val |= 0x505;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_5, val);
	} else {
		/* Set switchers in Auto in NORMAL mode & STANDBY mode for Atlas 2.0a */
		/* Setup the switcher mode for SW1 & SW2*/
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_4, &val);
		val &= ~0x3c0f;
		val |= 0x2008;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_4, val);

		/* Setup the switcher mode for SW3 & SW4 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_5, &val);
		val &= ~0xf0f;
		val |= 0x808;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_5, val);
	}

	/* Set VDIG to 1.65V, VGEN3 to 1.8V, VCAM to 2.5V */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_SETTING_0, &val);
	val &= ~0x34030;
	val |= 0x10020;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_SETTING_0, val);

	/* Set VVIDEO to 2.775V, VAUDIO to 3V, VSD to 3.15V */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_SETTING_1, &val);
	val &= ~0x1FC;
	val |= 0x1F4;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_SETTING_1, val);

	/* Configure VGEN3 and VCAM regulators to use external PNP */
	val = 0x208;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_MODE_1, val);

	udelay(200);

	/* Enable VGEN3, VCAM, VAUDIO, VVIDEO, VSD regulators */
	val = 0x49249;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_MODE_1, val);

	udelay(200);

	pr_info("initialized PMIC\n");

	console_flush();
	imx51_init_lowlevel(800);
	clock_notifier_call_chain();
}

static int zii_rdu1_init(void)
{
	if (!of_machine_is_compatible("zii,imx51-rdu1"))
		return 0;

	barebox_set_hostname("rdu1");

	mc13xxx_register_init_callback(zii_rdu1_power_init);

	imx51_bbu_internal_mmc_register_handler("mmc",
		"/dev/mmc0", 0);
	imx51_bbu_internal_spi_i2c_register_handler("spi",
		"/dev/dataflash0.barebox",
		BBU_HANDLER_FLAG_DEFAULT | BBU_HANDLER_SKIP_HEADER_OFFSET);

	return 0;
}
coredevice_initcall(zii_rdu1_init);

static struct device_d sndev;

static struct rdu1_display_type {
	const char *substr;	/* how it is described in spinor */
	const char *mode_name;	/* mode_name for barebox DT */
	const char *compatible;	/* compatible for kernel DT */
} display_types[] = {
	{ "Toshiba89", "toshiba89", "toshiba,lt089ac29000" },
	{ "CHIMEI15", "chimei15", "innolux,g154i1-le1" }
}, *current_dt; 

static int add_sndev_params(void)
{
	void *buf;
	size_t size;
	int ret;
	char *p, *v, *n, *e;

	ret = read_file_2("/dev/dataflash0", &size, &buf, 1024);
	if (ret && ret != -EFBIG) {
		printf("Failed to read settings from spi flash\n");
		return ret;
	}

	p = buf;
	e = buf + size;

	while (p != e && *p) {
		v = p + strnlen(p, e - p);
		if (v == e)
			break;
		v++;
		n = v + strnlen(v, e - v);
		if (n == e)
			break;
		n++;

		dev_add_param_fixed(&sndev, p, v);

		p = n;
	}

	free(buf);

	return 0;
}

static int set_fb0_mode(void)
{
	const char *video;
	struct device_d *fb0;
	struct rdu1_display_type;
	int i, ret;

	video = dev_get_param(&sndev, "video");
	if (!video) {
		dev_warn(&sndev, "spinor.video is not defined\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(display_types); i++) {
		if (strstr(video, display_types[i].substr)) {
			current_dt = &display_types[i];
			break;
		}
	}

	if (!current_dt) {
		dev_warn(&sndev, "unsupported value of spinor.video\n");
		return -EINVAL;
	}

	fb0 = get_device_by_name("fb0");
	if (!fb0) {
		dev_warn(&sndev, "fb0 device not found\n");
		return -EINVAL;
	}

	ret = dev_set_param(fb0, "mode_name", current_dt->mode_name);
	if (ret) {
		dev_warn(fb0, "failed to set fb0.mode_name to \'%s\'\n",
				current_dt->mode_name);
		return ret;
	}

	return 0;
}

static int fixup_display(struct device_node *root, void *context)
{
	struct device_node *node;

	pr_info("Enabling panel with compatible \"%s\".\n",
		current_dt->compatible);

	node = of_find_node_by_name(root, "panel");
	if (!node)
		return -ENODEV;

	of_device_enable(node);
	of_set_property(node, "compatible", current_dt->compatible,
			strlen(current_dt->compatible) + 1, 1);

	return 0;
}

static char *part_number;

static void fetch_part_number(void)
{
	int fd, ret;
	uint8_t buf[0x21];

	fd = open_and_lseek("/dev/nvmem1", O_RDONLY, 0x20);
	if (fd < 0)
		return;

	ret = read(fd, buf, 32);
	if (ret < 32)
		return;

	if (buf[0] < 1 || buf[0] > 31)
		return;

	buf[buf[0]+1] = 0;
	part_number = strdup(&buf[1]);
	if (part_number)
		printf("RDU1 part number: %s\n", part_number);
}

static int fixup_touchscreen(struct device_node *root, void *context)
{
	struct device_node *i2c2_node, *node;
	int i;

	static struct {
		const char *part_number_prefix;
		const char *node_name;
	} table[] = {
		{ "00-5103-01", "touchscreen@4c" },
		{ "00-5103-30", "touchscreen@20" },
		{ "00-5103-31", "touchscreen@20" },
		{ "00-5105-01", "touchscreen@4b" },
		{ "00-5105-20", "touchscreen@4b" },
		{ "00-5105-30", "touchscreen@20" },
		{ "00-5107-01", "touchscreen@4b" },
		{ "00-5108-01", "touchscreen@4c" },
		{ "00-5118-30", "touchscreen@20" },
	};

	if (!part_number)
		return 0;

	for (i = 0; i < ARRAY_SIZE(table); i++) {
		const char *prefix = table[i].part_number_prefix;
		if (!strncmp(part_number, prefix, strlen(prefix)))
			break;
	}

	if (i == ARRAY_SIZE(table))
		return 0;

	pr_info("Enabling \"%s\".\n", table[i].node_name);

	i2c2_node = of_find_node_by_alias(root, "i2c1");   /* i2c1 = &i2c2 */
	if (!i2c2_node)
		return 0;

	node = of_find_node_by_name(i2c2_node, table[i].node_name);
	if (!node)
		return 0;

	of_device_enable(node);
	return 0;
}

static int rdu1_late_init(void)
{
	int ret;

	if (!of_machine_is_compatible("zii,imx51-rdu1"))
		return 0;

	strcpy(sndev.name, "spinor");
	sndev.id = DEVICE_ID_SINGLE;

	ret = register_device(&sndev);
	if (ret < 0)
		return ret;

	ret = add_sndev_params();
	if (ret)
		return ret;

	ret = set_fb0_mode();
	if (ret)
		return ret;

	ret = of_register_fixup(fixup_display, NULL);
	if (ret)
		return ret;

	fetch_part_number();
	return of_register_fixup(fixup_touchscreen, NULL);
}
late_initcall(rdu1_late_init);

#ifdef CONFIG_ARCH_IMX_XLOAD

static int zii_rdu1_xload_init_pinmux(void)
{
	static const iomux_v3_cfg_t pinmux[] = {
		/* (e)CSPI */
		MX51_PAD_CSPI1_MOSI__ECSPI1_MOSI,
		MX51_PAD_CSPI1_MISO__ECSPI1_MISO,
		MX51_PAD_CSPI1_SCLK__ECSPI1_SCLK,

		/* (e)CSPI chip select lines */
		MX51_PAD_CSPI1_SS1__GPIO4_25,


		/* eSDHC 1 */
		MX51_PAD_SD1_CMD__SD1_CMD,
		MX51_PAD_SD1_CLK__SD1_CLK,
		MX51_PAD_SD1_DATA0__SD1_DATA0,
		MX51_PAD_SD1_DATA1__SD1_DATA1,
		MX51_PAD_SD1_DATA2__SD1_DATA2,
		MX51_PAD_SD1_DATA3__SD1_DATA3,
	};

	mxc_iomux_v3_setup_multiple_pads(ARRAY_AND_SIZE(pinmux));

	return 0;
}
coredevice_initcall(zii_rdu1_xload_init_pinmux);

static int zii_rdu1_xload_init_devices(void)
{
	static int spi0_chipselects[] = {
		IMX_GPIO_NR(4, 25),
	};

	static struct spi_imx_master spi0_pdata = {
		.chipselect = spi0_chipselects,
		.num_chipselect = ARRAY_SIZE(spi0_chipselects),
	};

	static const struct spi_board_info spi0_devices[] = {
		{
			.name		= "mtd_dataflash",
			.chip_select	= 0,
			.max_speed_hz	= 25 * 1000 * 1000,
			.bus_num	= 0,
		},
	};

	imx51_add_mmc0(NULL);

	spi_register_board_info(ARRAY_AND_SIZE(spi0_devices));
	imx51_add_spi0(&spi0_pdata);

	return 0;
}
device_initcall(zii_rdu1_xload_init_devices);

#endif