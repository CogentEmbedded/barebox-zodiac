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
#include <linux/nvmem-consumer.h>

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

static void *
rdu2_nvmem_cell_read(struct device_node *np, const char *cell_name, int bytes)
{
	struct nvmem_cell *cell;
	size_t len;
	void *value;

	cell = of_nvmem_cell_get(np, cell_name);
	if (IS_ERR(cell)) {
		return cell;
	}

	value = nvmem_cell_read(cell, &len);
	if (len != bytes) {
		kfree(value);
		return ERR_PTR(-EINVAL);
	}

	return value;
}

/*
 * FIXME: Needs to be made into convenience function and share with
 * OCOTP driver
 */
static void memreverse(void *dest, const void *src, size_t n)
{
	char *destp = dest;
	const char *srcp = src + n - 1;

	while(n--)
		*destp++ = *srcp--;
}

static int rdu2_eth_register_ethaddr(struct device_node *np)
{
	u8 *__mac, mac[6];

	__mac = rdu2_nvmem_cell_read(np, "mac-address", sizeof(mac));
	if (IS_ERR(__mac)) {
		pr_err("Failed to fread MAC address for ethernet0\n");
		return PTR_ERR(__mac);
	}

	memreverse(mac, __mac, sizeof(mac));
	kfree(__mac);
	of_eth_register_ethaddr(np, mac);

	return 0;
}

static int rdu2_ethernet0_init(void)
{
	struct device_node *np, *root;

	if (!of_machine_is_compatible("zii,imx6q-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx6qp-zii-rdu2"))
		return 0;

	root = of_get_root_node();
	np   = of_find_node_by_alias(root, "ethernet0");
	if (!np)
		return -ENODEV;

	return rdu2_eth_register_ethaddr(np);
}
late_initcall(rdu2_ethernet0_init);

static int rdu2_ethernet1_init(void)
{
	struct device_node *np, *root;

	if (!of_machine_is_compatible("zii,imx6q-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx6qp-zii-rdu2"))
		return 0;

	root = of_get_root_node();
	np   = of_find_node_by_alias(root, "ethernet1");
	if (!np)
		return -ENODEV;

	return rdu2_eth_register_ethaddr(np);
}
late_initcall(rdu2_ethernet1_init);

