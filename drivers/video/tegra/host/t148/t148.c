/*
 * drivers/video/tegra/host/t148/t148.c
 *
 * Tegra Graphics Init for T148 Architecture Chips
 *
 * Copyright (c) 2012-2014, NVIDIA Corporation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/export.h>
#include <linux/mutex.h>
#include <linux/tegra-powergate.h>
#include <linux/nvhost.h>

#include <mach/mc.h>

#include "dev.h"
#include "class_ids.h"
#include "host1x/host1x_cdma.h"
#include "t148/t148.h"
#include "t114/t114.h"
#include "host1x/host1x03_hardware.h"
#include "gr2d/gr2d_t114.h"
#include "gr3d/gr3d.h"
#include "gr3d/gr3d_t114.h"
#include "gr3d/scale3d.h"
#include "nvhost_scale.h"
#include "msenc/msenc.h"
#include "tsec/tsec.h"
#include "linux/nvhost_ioctl.h"
#include "nvhost_channel.h"
#include "chip_support.h"
#include "class_ids.h"

/* HACK! This needs to come from DT */
#include "../../../../../arch/arm/mach-tegra/iomap.h"

static int t148_num_alloc_channels = 0;

static struct resource tegra_host1x03_resources[] = {
	{
		.start = TEGRA_HOST1X_BASE,
		.end = TEGRA_HOST1X_BASE + TEGRA_HOST1X_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = INT_HOST1X_MPCORE_SYNCPT,
		.end = INT_HOST1X_MPCORE_SYNCPT,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = INT_HOST1X_MPCORE_GENERAL,
		.end = INT_HOST1X_MPCORE_GENERAL,
		.flags = IORESOURCE_IRQ,
	},
};

static const char *s_syncpt_names[48] = {
	"gfx_host",
	"", "", "", "", "", "", "",
	"disp0_a", "disp1_a", "avp_0",
	"csi_vi_0", "csi_vi_1",
	"vi_isp_0", "vi_isp_1", "vi_isp_2", "vi_isp_3", "vi_isp_4",
	"2d_0", "2d_1",
	"disp0_b", "disp1_b",
	"3d",
	"msenc",
	"disp0_c", "disp1_c",
	"vblank0", "vblank1",
	"tsec", "msenc_unused",
	"2d_tinyblt",
	"dsi"
};

static struct host1x_device_info host1x03_info = {
	.nb_channels	= 12,
	.nb_pts		= 48,
	.nb_mlocks	= 16,
	.nb_bases	= 12,
	.syncpt_names	= s_syncpt_names,
	.client_managed	= NVSYNCPTS_CLIENT_MANAGED,
};

struct nvhost_device_data t14_host1x_info = {
	.clocks		= { {"host1x", 81600000} },
	NVHOST_MODULE_NO_POWERGATE_IDS,
	.private_data	= &host1x03_info,
};

static struct platform_device tegra_host1x03_device = {
	.name		= "host1x",
	.id		= -1,
	.resource	= tegra_host1x03_resources,
	.num_resources	= ARRAY_SIZE(tegra_host1x03_resources),
	.dev		= {
		.platform_data = &t14_host1x_info,
	},
};

struct nvhost_device_data t14_gr3d_info = {
	.version	= 3,
	.index		= 1,
	.syncpts	= {NVSYNCPT_3D},
	.waitbases	= {NVWAITBASE_3D},
	.modulemutexes	= {NVMODMUTEX_3D},
	.class		= NV_GRAPHICS_3D_CLASS_ID,
	.clocks		= { {"gr3d", UINT_MAX, 8, TEGRA_MC_CLIENT_NV},
			    {"emc", UINT_MAX, 75} },
	.powergate_ids	= { TEGRA_POWERGATE_3D, -1 },
	NVHOST_DEFAULT_CLOCKGATE_DELAY,
	.can_powergate	= true,
	.powergate_delay = 250,
	.powerup_reset	= true,
	.moduleid	= NVHOST_MODULE_NONE,

	.busy		= nvhost_scale_notify_busy,
	.idle		= nvhost_scale_notify_idle,
	.init		= nvhost_scale_hw_init,
	.deinit		= nvhost_scale_hw_deinit,
	.scaling_init	= nvhost_scale3d_init,
	.scaling_deinit	= nvhost_scale3d_deinit,
	.scaling_post_cb = &nvhost_scale3d_callback,
	.devfreq_governor = "nvhost_podgov",
	.actmon_enabled	= true,
	.gpu_edp_device	= true,

	.suspend_ndev	= nvhost_scale3d_suspend,
	.prepare_poweroff = nvhost_gr3d_t114_prepare_power_off,
	.finalize_poweron = nvhost_gr3d_t114_finalize_power_on,
	.alloc_hwctx_handler = nvhost_gr3d_t114_ctxhandler_init,
};

static struct platform_device tegra_gr3d03_device = {
	.name		= "gr3d",
	.id		= -1,
	.dev		= {
		.platform_data = &t14_gr3d_info,
	},
};

struct nvhost_device_data t14_gr2d_info = {
	.index		= 2,
	.syncpts	= {NVSYNCPT_2D_0, NVSYNCPT_2D_1},
	.waitbases	= {NVWAITBASE_2D_0, NVWAITBASE_2D_1},
	.modulemutexes	= {NVMODMUTEX_2D_FULL, NVMODMUTEX_2D_SIMPLE,
			  NVMODMUTEX_2D_SB_A, NVMODMUTEX_2D_SB_B},
	.clocks		= { {"gr2d", 0, 7, TEGRA_MC_CLIENT_G2},
			    {"epp", 0, 10, TEGRA_MC_CLIENT_EPP},
			    {"emc", 300000000, 75 } },
	.powergate_ids	= { TEGRA_POWERGATE_HEG, -1 },
	.clockgate_delay = 0,
	.can_powergate  = true,
	.powergate_delay = 100,
	.moduleid	= NVHOST_MODULE_NONE,
	.serialize	= true,
	.finalize_poweron = nvhost_gr2d_t114_finalize_poweron,
};

static struct platform_device tegra_gr2d03_device = {
	.name		= "gr2d",
	.id		= -1,
	.dev		= {
		.platform_data = &t14_gr2d_info,
	},
};

static struct resource isp_resources[] = {
	{
		.name = "regs",
		.start = TEGRA_ISP_BASE,
		.end = TEGRA_ISP_BASE + TEGRA_ISP_SIZE - 1,
		.flags = IORESOURCE_MEM,
	}
};

struct nvhost_device_data t14_isp_info = {
	.index		= 3,
	.syncpts	= {NVSYNCPT_VI_ISP_2, NVSYNCPT_VI_ISP_3,
			  NVSYNCPT_VI_ISP_4},
	NVHOST_MODULE_NO_POWERGATE_IDS,
	NVHOST_DEFAULT_CLOCKGATE_DELAY,
	.moduleid	= NVHOST_MODULE_ISP,
};

static struct platform_device tegra_isp01_device = {
	.name		= "isp",
	.id		= -1,
	.resource	= isp_resources,
	.num_resources	= ARRAY_SIZE(isp_resources),
	.dev		= {
		.platform_data = &t14_isp_info,
	},
};

static struct resource vi_resources[] = {
	{
		.name = "regs",
		.start = TEGRA_VI_BASE,
		.end = TEGRA_VI_BASE + TEGRA_VI_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "irq",
		.start = INT_VI_GENERAL,
		.end = INT_VI_GENERAL,
		.flags = IORESOURCE_IRQ,
	},
};

struct nvhost_device_data t14_vi_info = {
	.index		= 4,
	.syncpts	= {NVSYNCPT_CSI_VI_0, NVSYNCPT_CSI_VI_1,
			  NVSYNCPT_VI_ISP_0, NVSYNCPT_VI_ISP_1,
			  NVSYNCPT_VI_ISP_2, NVSYNCPT_VI_ISP_3,
			  NVSYNCPT_VI_ISP_4},
	.modulemutexes	= {NVMODMUTEX_VI_0},
	.clocks		= { {"host1x", 136000000, 6} },
	.exclusive	= true,
	NVHOST_MODULE_NO_POWERGATE_IDS,
	NVHOST_DEFAULT_CLOCKGATE_DELAY,
	.moduleid	= NVHOST_MODULE_VI,
	.update_clk	= nvhost_host1x_update_clk,
};
EXPORT_SYMBOL(t14_vi_info);

static struct platform_device tegra_vi01_device = {
	.name		= "vi",
	.id		= -1,
	.resource	= vi_resources,
	.num_resources	= ARRAY_SIZE(vi_resources),
	.dev		= {
		.platform_data = &t14_vi_info,
	},
};

static struct resource msenc_resources[] = {
	{
		.name = "regs",
		.start = TEGRA_MSENC_BASE,
		.end = TEGRA_MSENC_BASE + TEGRA_MSENC_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct nvhost_device_data t14_msenc_info = {
	.version	= NVHOST_ENCODE_MSENC_VER(3, 0),
	.index		= 5,
	.syncpts	= {NVSYNCPT_MSENC},
	.waitbases	= {NVWAITBASE_MSENC},
	.class		= NV_VIDEO_ENCODE_MSENC_CLASS_ID,
	.clocks		= { {"msenc", UINT_MAX, 107, TEGRA_MC_CLIENT_MSENC},
			    {"emc", 300000000, 75} },
	.powergate_ids = { TEGRA_POWERGATE_MPE, -1 },
	NVHOST_DEFAULT_CLOCKGATE_DELAY,
	.powergate_delay = 100,
	.can_powergate = true,
	.moduleid	= NVHOST_MODULE_MSENC,
	.powerup_reset	= true,
	.init           = nvhost_msenc_init,
	.deinit         = nvhost_msenc_deinit,
	.finalize_poweron = nvhost_msenc_finalize_poweron,
};

static struct platform_device tegra_msenc03_device = {
	.name		= "msenc",
	.id		= -1,
	.resource	= msenc_resources,
	.num_resources	= ARRAY_SIZE(msenc_resources),
	.dev		= {
		.platform_data = &t14_msenc_info,
	},
};

static struct resource tsec_resources[] = {
	{
		.name = "regs",
		.start = TEGRA_TSEC_BASE,
		.end = TEGRA_TSEC_BASE + TEGRA_TSEC_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct nvhost_device_data t14_tsec_info = {
	.version	= NVHOST_ENCODE_TSEC_VER(1,0),
	.index		= 7,
	.syncpts	= {NVSYNCPT_TSEC},
	.waitbases	= {NVWAITBASE_TSEC},
	.class		= NV_TSEC_CLASS_ID,
	.exclusive	= false,
	.clocks		= { {"tsec", UINT_MAX, 108, TEGRA_MC_CLIENT_TSEC},
			    {"emc", 300000000, 75} },
	NVHOST_MODULE_NO_POWERGATE_IDS,
	NVHOST_DEFAULT_CLOCKGATE_DELAY,
	.moduleid	= NVHOST_MODULE_TSEC,
	.init          = nvhost_tsec_init,
};

static struct platform_device tegra_tsec01_device = {
	.name		= "tsec",
	.id		= -1,
	.resource	= tsec_resources,
	.num_resources	= ARRAY_SIZE(tsec_resources),
	.dev		= {
		.platform_data = &t14_tsec_info,
	},
};

static struct platform_device *t14_devices[] = {
	&tegra_gr3d03_device,
	&tegra_gr2d03_device,
	&tegra_isp01_device,
	&tegra_vi01_device,
	&tegra_msenc03_device,
	&tegra_tsec01_device,
};

struct platform_device *tegra14_register_host1x_devices(void)
{
	int index = 0;
	struct platform_device *pdev;

	/* register host1x device first */
	platform_device_register(&tegra_host1x03_device);
	tegra_host1x03_device.dev.parent = NULL;

	/* register clients with host1x device as parent */
	for (index = 0; index < ARRAY_SIZE(t14_devices); index++) {
		pdev = t14_devices[index];
		pdev->dev.parent = &tegra_host1x03_device.dev;
		platform_device_register(pdev);
	}

	return &tegra_host1x03_device;
}

#include "host1x/host1x_channel.c"
#include "host1x/host1x_cdma.c"
#include "host1x/host1x_debug.c"
#include "host1x/host1x_syncpt.c"
#include "host1x/host1x_intr.c"
#include "host1x/host1x_actmon_t114.c"

static void t148_free_nvhost_channel(struct nvhost_channel *ch)
{
	nvhost_free_channel_internal(ch, &t148_num_alloc_channels);
}

static struct nvhost_channel *t148_alloc_nvhost_channel(
		struct platform_device *dev)
{
	struct nvhost_device_data *pdata = platform_get_drvdata(dev);
	struct nvhost_channel *ch = nvhost_alloc_channel_internal(pdata->index,
		nvhost_get_host(dev)->info.nb_channels,
		&t148_num_alloc_channels);
	if (ch)
		ch->ops = host1x_channel_ops;
	return ch;
}

int nvhost_init_t148_support(struct nvhost_master *host,
	struct nvhost_chip_support *op)
{
	op->cdma = host1x_cdma_ops;
	op->push_buffer = host1x_pushbuffer_ops;
	op->debug = host1x_debug_ops;
	host->sync_aperture = host->aperture + HOST1X_CHANNEL_SYNC_REG_BASE;
	op->syncpt = host1x_syncpt_ops;
	op->intr = host1x_intr_ops;
	op->nvhost_dev.alloc_nvhost_channel = t148_alloc_nvhost_channel;
	op->nvhost_dev.free_nvhost_channel = t148_free_nvhost_channel;
	op->actmon = host1x_actmon_ops;

	return 0;
}
