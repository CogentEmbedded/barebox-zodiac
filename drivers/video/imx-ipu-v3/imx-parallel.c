/*
 * i.MX drm driver - parallel display implementation
 *
 * Copyright (C) 2012 Sascha Hauer, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <common.h>
#include <fb.h>
#include <io.h>
#include <of_graph.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <init.h>
#include <video/vpl.h>
#include <mfd/imx6q-iomuxc-gpr.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <asm-generic/div64.h>
#include <linux/clk.h>
#include <mach/imx6-regs.h>
#include <mach/imx53-regs.h>

#include "imx-ipu-v3.h"
#include "ipuv3-plane.h"

#define LDB_CH0_MODE_EN_TO_DI0		(1 << 0)
#define LDB_CH0_MODE_EN_TO_DI1		(3 << 0)
#define LDB_CH0_MODE_EN_MASK		(3 << 0)
#define LDB_CH1_MODE_EN_TO_DI0		(1 << 2)
#define LDB_CH1_MODE_EN_TO_DI1		(3 << 2)
#define LDB_CH1_MODE_EN_MASK		(3 << 2)
#define LDB_SPLIT_MODE_EN		(1 << 4)
#define LDB_DATA_WIDTH_CH0_24		(1 << 5)
#define LDB_BIT_MAP_CH0_JEIDA		(1 << 6)
#define LDB_DATA_WIDTH_CH1_24		(1 << 7)
#define LDB_BIT_MAP_CH1_JEIDA		(1 << 8)
#define LDB_DI0_VS_POL_ACT_LOW		(1 << 9)
#define LDB_DI1_VS_POL_ACT_LOW		(1 << 10)
#define LDB_BGREF_RMODE_INT		(1 << 15)

struct imx_pd {
	struct device_d *dev;
	struct vpl vpl;
	u32 bus_format;
	struct display_timings *timings;
	struct device_node *remote;
};

static int imx_pd_get_modes(struct imx_pd *imx_pd,
				struct display_timings *timings)
{
	printf("imx_pd_get_modes\n");
	if (imx_pd->timings) {
		printf(" got timings\n");
		timings->num_modes = imx_pd->timings->num_modes;
		timings->native_mode = imx_pd->timings->native_mode;
		timings->modes = imx_pd->timings->modes;
		timings->edid = NULL;
		return 0;
	}

	/* if no timing provided assume it is provided by next node */
	printf(" asking next vpl node\n");
	return vpl_ioctl(&imx_pd->vpl, 1, VPL_GET_VIDEOMODES, timings);
}

static int imx_pd_ioctl(struct vpl *vpl, unsigned int port,
		unsigned int cmd, void *data)
{
	struct imx_pd *imx_pd = container_of(vpl,
			struct imx_pd, vpl);
	struct ipu_di_mode *mode;

	switch (cmd) {
	case IMX_IPU_VPL_DI_MODE:
		mode = data;

		mode->di_clkflags = IPU_DI_CLKMODE_SYNC;
		mode->interface_pix_fmt = V4L2_PIX_FMT_RGB24;
		return 0;
	case VPL_GET_VIDEOMODES:
		return imx_pd_get_modes(imx_pd, data);
	default:
		return 0;
	}
}

static int imx_pd_probe(struct device_d *dev)
{
	struct device_node *np = dev->device_node;
	struct device_node *endpoint;
	struct device_node *port;
	struct imx_pd *imx_pd;
	const char *fmt;
	int ret;

	dev_dbg(dev, "%s enter\n", __func__);

	imx_pd = xzalloc(sizeof(*imx_pd));

	ret = of_property_read_string(np, "interface-pix-fmt", &fmt);
	if (!ret) {
		if (!strcmp(fmt, "rgb24"))
			imx_pd->bus_format = DRM_FORMAT_ABGR8888;
		else if (!strcmp(fmt, "rgb565"))
			imx_pd->bus_format = DRM_FORMAT_RGB565;
		else if (!strcmp(fmt, "bgr666"))
			imx_pd->bus_format = DRM_FORMAT_BGR565;
		else {
			dev_err(dev, "Unknown interface-pix-fmt");
			return -EINVAL;
		}
	}

	port = of_graph_get_port_by_id(np, 0);
	if (!port) {
		dev_err(dev, "No port 0\n");
		return -ENODEV;
	}

	endpoint = of_get_child_by_name(port, "endpoint");
	if (!endpoint) {
		dev_warn(dev, "No endpoint found on %s\n", port->full_name);
		return -EINVAL;
	}

	imx_pd->timings = of_get_display_timings(np);
	if (!imx_pd->timings) {
		port = of_graph_get_port_by_id(np, 1);

		if (!port) {
			dev_err(dev, "No timing and no port 1\n");
			return -ENODEV;
		}

		endpoint = of_get_child_by_name(port, "endpoint");
		if (!endpoint) {
			dev_warn(dev, "No endpoint on port 1 and no timing\n");
			return -EINVAL;
		}
	}

	imx_pd->vpl.node = np;
	imx_pd->vpl.ioctl = &imx_pd_ioctl;
	ret = vpl_register(&imx_pd->vpl);
	if (ret) {
		dev_err(dev, "VPL register failed port 0\n");
		return ret;
	}

	dev_dbg(dev, "registered vpl for %s\n", np->full_name);

	return 0;
}

static struct of_device_id imx_pd_dt_ids[] = {
	{ .compatible = "fsl,imx-parallel-display",},
	{ /* sentinel */ }
};

static struct driver_d imx_pd_driver = {
	.probe		= imx_pd_probe,
	.of_compatible	= imx_pd_dt_ids,
	.name		= "imx-parallel-display",
};
device_platform_driver(imx_pd_driver);

MODULE_DESCRIPTION("i.MX LVDS display driver");
MODULE_AUTHOR("Sascha Hauer, Pengutronix");
MODULE_LICENSE("GPL");
