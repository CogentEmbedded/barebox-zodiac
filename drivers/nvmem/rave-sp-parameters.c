/*
 * EEPROM driver for RAVE SP
 *
 * Copyright (C) 2017 Zodiac Inflight Innovations
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <of_device.h>
#include <malloc.h>
#include <linux/sizes.h>
#include <linux/mfd/rave-sp.h>
#include <linux/nvmem-provider.h>

#define RAVE_SP_PARAMETERS_MAX_CMD_SIZE	10
#define RAVE_SP_PARAMETERS_MAX_RSP_SIZE	10

enum {
	RAVE_SP_PARAM_IP_ADDR_SIZE = 8,
	RAVE_SP_PARAM_LCD_TYPE_SIZE = 1,

	/*
	 * Should be the sum of all above
	 */
	RAVE_SP_PARAM_SIZE = RAVE_SP_PARAM_IP_ADDR_SIZE +
			     RAVE_SP_PARAM_LCD_TYPE_SIZE
};

static int rave_sp_parameters_read(struct device_d *dev, const int offset,
				   void *val, int bytes)
{
	struct rave_sp *sp = dev->parent->priv;
	u8 cmd[RAVE_SP_PARAMETERS_MAX_CMD_SIZE];
	u8 rsp[RAVE_SP_PARAMETERS_MAX_RSP_SIZE];
	int cmd_size, rsp_size;
	int ret;

	/*
	 * Every command would have a command code and ack ID
	 */
	cmd_size = sizeof(cmd[0]) + sizeof(cmd[1]);

	switch (offset) {
	case 0:
		cmd[0] = RAVE_SP_CMD_REQ_IP_ADDR;
		cmd[1] = 0;
		cmd[2] = 0; 	/* FIXME: Support for RJU? */
		cmd[3] = 0;	/* Add support for IPs other than "self" */

		cmd_size += sizeof(cmd[2]) + sizeof(cmd[3]);
		rsp_size = RAVE_SP_PARAM_IP_ADDR_SIZE;
		break;

	case 8:
		cmd[0] = RAVE_SP_CMD_LCD_TYPE;
		cmd[1] = 0;
		rsp_size = RAVE_SP_PARAM_LCD_TYPE_SIZE;
		break;
	default:
		return -EINVAL;
	}

	if (bytes > rsp_size)
		return -EINVAL;

	ret = rave_sp_exec(sp, cmd, cmd_size, rsp, rsp_size);
	if (ret)
		return ret;

	memcpy(val, rsp, bytes);
	return 0;
}

static int rave_sp_parameters_write(struct device_d *dev, const int offset,
				const void *val, int bytes)
{
	return -ENOTSUPP;
}

static const struct nvmem_bus rave_sp_parameters_nvmem_bus = {
	.write = rave_sp_parameters_write,
	.read  = rave_sp_parameters_read,
};

static int rave_sp_parameters_probe(struct device_d *dev)
{
	struct rave_sp *sp = dev->parent->priv;
	struct nvmem_config config = { 0 };
	struct nvmem_device *nvmem;

	dev->priv = sp;

	config.name      = dev_name(dev);
	config.dev       = dev;
	config.word_size = 1;
	config.stride    = 1;
	config.size      = RAVE_SP_PARAM_SIZE;
	config.bus       = &rave_sp_parameters_nvmem_bus;

	nvmem = nvmem_register(&config);
	if (IS_ERR(nvmem))
		return PTR_ERR(nvmem);

	return 0;
}

static __maybe_unused const
struct of_device_id rave_sp_parameters_of_match[] = {
	{ .compatible = "zii,rave-sp-parameters" },
	{}
};

static struct driver_d rave_sp_parameters_driver = {
	.name = "rave-sp-parameters",
	.probe = rave_sp_parameters_probe,
	.of_compatible = DRV_OF_COMPAT(rave_sp_parameters_of_match),
};
console_platform_driver(rave_sp_parameters_driver);
