// SPDX-License-Identifier: GPL-2.0+

/*
 * Copyright (C) 2017 Zodiac Inflight Innovation
 *
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 *
 * based on previous iterations of this code
 *
 *     Copyright (C) 2015 Nikita Yushchenko, CogentEmbedded, Inc
 *     Copyright (C) 2015 Andrey Gusakov, CogentEmbedded, Inc
 *
 * based on similar i.MX51 EVK (Babbage) board support code
 *
 *     Copyright (C) 2007 Sascha Hauer, Pengutronix
 */

#include <common.h>
#include <fcntl.h>
#include <fs.h>
#include <init.h>
#include <libfile.h>
#include <mach/bbu.h>
#include <mach/imx5.h>

static int zii_rdu1_init(void)
{
	if (!of_machine_is_compatible("zii,imx51-rdu1"))
		return 0;

	imx51_babbage_power_init();

	barebox_set_hostname("rdu1");

	imx51_bbu_internal_mmc_register_handler("mmc", "/dev/mmc0", 0);
	imx51_bbu_internal_spi_i2c_register_handler("spi",
		"/dev/dataflash0.barebox",
		BBU_HANDLER_FLAG_DEFAULT |
		IMX_BBU_FLAG_PARTITION_STARTS_AT_HEADER);

	return 0;
}
coredevice_initcall(zii_rdu1_init);

static struct rdu1_display_type {
	const char *substr;	/* how it is described in spinor */
	const char *mode_name;	/* mode_name for barebox DT */
	const char *compatible;	/* compatible for kernel DT */
} display_types[] = {
	{ "Toshiba89", "toshiba89", "toshiba,lt089ac29000" },
	{ "CHIMEI15", "chimei15", "innolux,g154i1-le1" }
}, *current_dt;

static int rdu1_fixup_display(struct device_node *root, void *context)
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

static void rdu1_display_setup(void)
{
	struct device_d *fb0;
	size_t size;
	void *buf;
	int ret, i, pos = 0;

	ret = read_file_2("/dev/dataflash0.config", &size, &buf, 512);
	if (ret && ret != -EFBIG) {
		pr_err("Failed to read settings from SPI flash!\n");
		return;
	}

	while (pos < size) {
		char *cur = buf + pos;

		pos += strlen(cur) + 1;

		if (strstr(cur, "video"))
			break;
	}

	/* Sanity check before we try any further detection */
	if (pos >= size) {
		pr_err("Config flash doesn't contain video setting!\n");
		free(buf);
		return;
	}

	for (i = 0; i < ARRAY_SIZE(display_types); i++) {
		if (strstr(buf + pos, display_types[i].substr)) {
			current_dt = &display_types[i];
			break;
		}
	}

	free(buf);

	if (!current_dt) {
		pr_warn("No known display configuration found!\n");
		return;
	}

	fb0 = get_device_by_name("fb0");
	if (!fb0) {
		pr_warn("fb0 device not found!\n");
		return;
	}

	ret = dev_set_param(fb0, "mode_name", current_dt->mode_name);
	if (ret) {
		pr_warn("failed to set fb0.mode_name to \'%s\'\n",
			current_dt->mode_name);
		return;
	}

	of_register_fixup(rdu1_fixup_display, NULL);
}

static char *part_number;

static void fetch_part_number(void)
{
	int fd, ret;
	uint8_t buf[0x21];

	fd = open_and_lseek("/dev/main-eeprom", O_RDONLY, 0x20);
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
		pr_info("RDU1 part number: %s\n", part_number);
}

static int rdu1_fixup_touchscreen(struct device_node *root, void *context)
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
	if (!of_machine_is_compatible("zii,imx51-rdu1"))
		return 0;

	rdu1_display_setup();

	fetch_part_number();
	of_register_fixup(rdu1_fixup_touchscreen, NULL);

	return 0;
}
late_initcall(rdu1_late_init);
