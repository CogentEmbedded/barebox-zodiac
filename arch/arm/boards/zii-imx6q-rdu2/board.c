/*
 * Copyright (C) 2016 Zodiac Inflight Innovation
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
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
 */

#include <common.h>
#include <envfs.h>
#include <gpio.h>
#include <i2c/i2c.h>
#include <init.h>
#include <mach/bbu.h>
#include <mach/imx6.h>
#include <net.h>
#include <zii/pic.h>

#define RDU2_DAC1_RESET	IMX_GPIO_NR(1, 0)
#define RDU2_DAC2_RESET	IMX_GPIO_NR(1, 2)
#define RDU2_RST_TOUCH	IMX_GPIO_NR(1, 7)
#define RDU2_NFC_RESET	IMX_GPIO_NR(1, 17)
#define RDU2_HPA1_SDn	IMX_GPIO_NR(1, 4)
#define RDU2_HPA2_SDn	IMX_GPIO_NR(1, 5)

static const struct gpio rdu2_reset_gpios[] = {
	{
		.gpio = RDU2_DAC1_RESET,
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "dac1-reset",
	},
	{
		.gpio = RDU2_DAC2_RESET,
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "dac2-reset",
	},
	{
		.gpio = RDU2_RST_TOUCH,
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "rst-touch#",
	},
	{
		.gpio = RDU2_NFC_RESET,
		.flags = GPIOF_OUT_INIT_HIGH,
		.label = "nfc-reset",
	},
	{
		.gpio = RDU2_HPA1_SDn,
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "hpa1-sd-n",
	},
	{
		.gpio = RDU2_HPA2_SDn,
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "hpa2n-sd-n",
	},
};

static int rdu2_reset_audio_touchscreen_nfc(void)
{
	int ret;

	if (!of_machine_is_compatible("zii,imx6q-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx6qp-zii-rdu2"))
		return 0;

	ret = gpio_request_array(rdu2_reset_gpios,
				 ARRAY_SIZE(rdu2_reset_gpios));
	if (ret) {
		pr_err("Failed to request RDU2 reset gpios: %s\n",
		       strerror(-ret));
		return ret;
	}

	mdelay(100);

	gpio_direction_output(RDU2_DAC1_RESET, 1);
	gpio_direction_output(RDU2_DAC2_RESET, 1);
	gpio_direction_output(RDU2_RST_TOUCH,  1);
	gpio_direction_output(RDU2_NFC_RESET,  0);
	gpio_direction_output(RDU2_HPA1_SDn,   1);
	gpio_direction_output(RDU2_HPA2_SDn,   1);

	mdelay(100);

	return 0;
}
/*
 * When this function is called "hog" pingroup in device tree needs to
 * be already initialized
 */
late_initcall(rdu2_reset_audio_touchscreen_nfc);

static const struct gpio rdu2_front_panel_usb_gpios[] = {
	{
		.gpio = IMX_GPIO_NR(3, 19),
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "usb-emulation",
	},
	{
		.gpio = IMX_GPIO_NR(3, 20),
		.flags = GPIOF_OUT_INIT_HIGH,
		.label = "usb-mode1",
	},
	{
		.gpio = IMX_GPIO_NR(3, 23),
		.flags = GPIOF_OUT_INIT_HIGH,
		.label = "usb-mode2",
	},
};

static int rdu2_enable_front_panel_usb(void)
{
	int ret;

	if (!of_machine_is_compatible("zii,imx6q-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx6qp-zii-rdu2"))
		return 0;

	ret = gpio_request_array(rdu2_front_panel_usb_gpios,
				 ARRAY_SIZE(rdu2_front_panel_usb_gpios));
	if (ret) {
		pr_err("Failed to request RDU2 front panel USB gpios: %s\n",
		       strerror(-ret));

	}

	return ret;
}
late_initcall(rdu2_enable_front_panel_usb);

static int rdu2_devices_init(void)
{
	struct i2c_client client;
	u8 reg;

	if (!of_machine_is_compatible("zii,imx6q-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx6qp-zii-rdu2"))
		return 0;

	/* reset the ethernet switch, so we can use it for net booting */
	client.adapter = i2c_get_adapter(1);
	if (client.adapter) {
		/* address of the switch watchdog microcontroller */
		client.addr = 0x38;
		reg = 0x78;
		/* set switch reset time to 100ms */
		i2c_write_reg(&client, 0x0a, &reg, 1);
		/* reset the switch */
		reg = 0x01;
		i2c_write_reg(&client, 0x0d, &reg, 1);
		/* issue dummy command to work around firmware bug */
		i2c_read_reg(&client, 0x01, &reg, 1);
	}

	barebox_set_hostname("rdu2");

	imx6_bbu_internal_spi_i2c_register_handler("SPI", "/dev/m25p0.barebox",
						   BBU_HANDLER_FLAG_DEFAULT);

	imx6_bbu_internal_mmc_register_handler("eMMC", "/dev/mmc3", 0);

	defaultenv_append_directory(defaultenv_rdu2);

	return 0;
}
device_initcall(rdu2_devices_init);

#ifdef CONFIG_CMD_ZODIAC_PIC
enum rdu2_lcd_interface_type {
	IT_SINGLE_LVDS,
	IT_DUAL_LVDS,
	IT_EDP
};

enum rdu2_lvds_busformat {
	BF_NONE,
	BF_JEIDA,
	BF_SPWG
};

struct rdu2_lcd_info {
	enum rdu2_lcd_interface_type type;
	enum rdu2_lvds_busformat bus_format;
	char *compatible;
};

static struct rdu2_lcd_info lcd_info[] = {
	{ IT_SINGLE_LVDS, BF_SPWG, "innolux,g121i1-l01" }, /* unknown panel fallback */
	{ IT_SINGLE_LVDS, BF_SPWG, "innolux,g121i1-l01" }, /* Innolux 10.1" */
	{ IT_SINGLE_LVDS, BF_SPWG, "nec,nl12880bc20-05" }, /* NEC 12.1" */
	{ IT_EDP, BF_NONE, "" }, /* NLT 11.6" */
	{ IT_DUAL_LVDS, BF_JEIDA, "auo,g133han01" }, /* AUO 13.3" */
	{ IT_DUAL_LVDS, BF_SPWG, "auo,g185han01" }, /* AUO 18.5" */
	{ IT_DUAL_LVDS, BF_SPWG, "nlt,nl192108ac18-02d" }, /* NLT 15.6" */
	{ IT_DUAL_LVDS, BF_JEIDA, "auo,p320hvn03" }, /* AUO 31.5" */
};

static int lcd_type;

static int rdu2_fixup_display_internal(void)
{
	struct device_node *np, *child, *tmp;
	const char *compatible = lcd_info[lcd_type].compatible;
	const char *bus_format = lcd_info[lcd_type].bus_format == BF_SPWG ? "spwg" : "jeida";

	/* If the panel is eDP, just enable the parallel output and eDP bridge */
	if (lcd_info[lcd_type].type == IT_EDP) {
		np = of_find_compatible_node(NULL, NULL,
					     "toshiba,tc358767");
		if (!np)
			return -ENODEV;
		of_device_enable(np);

		return 0;
	}

	/* LVDS panels need the correct timings */
	np = of_find_node_by_name(NULL, "panel");
	if (!np)
		return -ENODEV;
	of_device_enable_and_register(np);

	/* Delete all mode entries, which aren't suited for the current display */
	np = of_find_node_by_name(np, "display-timings");
	if (!np)
		return -ENODEV;
	for_each_child_of_node_safe(np, tmp, child) {
		if (!of_device_is_compatible(child, compatible))
			of_delete_node(child);
	}

	/* enable LDB channel 0 and set correct interface mode */
	np = of_find_compatible_node(NULL, NULL, "fsl,imx6q-ldb");
	if (!np)
		return -ENODEV;
	of_device_enable_and_register(np);
	if (lcd_info[lcd_type].type == IT_DUAL_LVDS)
		of_set_property(np, "fsl,dual-channel", NULL, 0, 1);
	np = of_find_node_by_name(np, "lvds-channel@0");
	if (!np)
		return -ENODEV;
	of_device_enable(np);
	of_property_write_string(np, "fsl,data-mapping", bus_format);

	return 0;
}

static int rdu2_fixup_display(struct device_node *root, void *context)
{
	struct device_node *np;
	const char *compatible = lcd_info[lcd_type].compatible;

	/* If the panel is eDP, just enable the parallel output and eDP bridge */
	if (lcd_info[lcd_type].type == IT_EDP) {
		pr_info("Found eDP display, enabling parallel output and eDP bridge.\n");
		np = of_find_compatible_node(root, NULL,
					     "fsl,imx-parallel-display");
		if (!np)
			return 0;
		of_device_enable(np);

		np = of_find_compatible_node(root, NULL,
					     "toshiba,tc358767");
		if (!np)
			return 0;
		of_device_enable(np);

		return 0;
	}

	/* LVDS panels need the correct compatible */
	pr_info("Found LVDS display, enabling %s channel LDB and panel with compatible \"%s\".\n",
		lcd_info[lcd_type].type == IT_DUAL_LVDS ? "dual" : "single",
		compatible);

	np = of_find_node_by_name(root, "panel");
	if (!np)
		return 0;
	of_device_enable(np);
	of_set_property(np, "compatible", compatible, strlen(compatible) +1, 1);

	/* enable LDB channel 0 and set correct interface mode */
	np = of_find_compatible_node(root, NULL, "fsl,imx6q-ldb");
	if (!np)
		return 0;
	of_device_enable(np);
	if (lcd_info[lcd_type].type == IT_DUAL_LVDS)
		of_set_property(np, "fsl,dual-channel", NULL, 0, 1);
	np = of_find_node_by_name(np, "lvds-channel@0");
	if (!np)
		return 0;
	of_device_enable(np);

	return 0;
}

/*
 * This initcall needs to be executed before coredevices, so we have a chance
 * to fix up the internal DT with the correct display information.
 */
static int imx6_zodiac_postmmu_init(void)
{
	struct console_device *cdev;
	struct device_node *np;
	u8 mac[6];

	for_each_console(cdev) {
		if (cdev->devname &&
		    !(strcmp(cdev->devname, "serial3"))) {
			printf("Init PIC on %s\n", cdev->devname);
			pic_init(cdev, RDU2_PIC_BAUD_RATE, PIC_HW_ID_RDU2);
			break;
		}
	}

	/* disable the PIC watchdog */
	pic_disable_watchdog();

	/* get display type and register DT fixup */
	lcd_type = pic_get_lcd_type();
	if (lcd_type < 0)
		return 0;

	if (lcd_type == 0xff)
		lcd_type = 0;

	rdu2_fixup_display_internal();
	of_register_fixup(rdu2_fixup_display, NULL);

	np = of_find_node_by_alias(of_get_root_node(), "ethernet0");
	if (np) {
		pic_get_mac_address(0, mac);
		of_eth_register_ethaddr(np, mac);
	}

	np = of_find_node_by_alias(of_get_root_node(), "ethernet1");
	if (np) {
		pic_get_mac_address(1, mac);
		of_eth_register_ethaddr(np, mac);
	}

	/* get IP configuration from DDS */
	do_pic_get_ip();

	return 0;
}
postmmu_initcall(imx6_zodiac_postmmu_init);
#endif
