/*
 * drivers/video/tegra/host/gk20a/ltc_common.c
 *
 * GK20A Graphics
 *
 * Copyright (c) 2011-2014, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/dma-mapping.h>
#include <linux/delay.h>

#include "gk20a.h"
#include "gr_gk20a.h"

#include "dev.h"

static int gk20a_determine_L2_size_bytes(struct gk20a *g)
{
	const u32 gpuid = GK20A_GPUID(g->gpu_characteristics.arch,
				      g->gpu_characteristics.impl);
	u32 lts_per_ltc;
	u32 ways;
	u32 sets;
	u32 bytes_per_line;
	u32 active_ltcs;
	u32 cache_size;

	u32 tmp;
	u32 active_sets_value;

	tmp = gk20a_readl(g, ltc_ltc0_lts0_tstg_cfg1_r());
	ways = hweight32(ltc_ltc0_lts0_tstg_cfg1_active_ways_v(tmp));

	active_sets_value = ltc_ltc0_lts0_tstg_cfg1_active_sets_v(tmp);
	if (active_sets_value == ltc_ltc0_lts0_tstg_cfg1_active_sets_all_v()) {
		sets = 64;
	} else if (active_sets_value ==
		 ltc_ltc0_lts0_tstg_cfg1_active_sets_half_v()) {
		sets = 32;
	} else if (active_sets_value ==
		 ltc_ltc0_lts0_tstg_cfg1_active_sets_quarter_v()) {
		sets = 16;
	} else {
		dev_err(dev_from_gk20a(g),
			"Unknown constant %u for active sets",
		       (unsigned)active_sets_value);
		sets = 0;
	}

	active_ltcs = g->gr.num_fbps;

	/* chip-specific values */
	switch (gpuid) {
	case GK20A_GPUID_GK20A:
		lts_per_ltc = 1;
		bytes_per_line = 128;
		break;

	default:
		dev_err(dev_from_gk20a(g), "Unknown GPU id 0x%02x\n",
			(unsigned)gpuid);
		lts_per_ltc = 0;
		bytes_per_line = 0;
	}

	cache_size = active_ltcs * lts_per_ltc * ways * sets * bytes_per_line;

	return cache_size;
}

/*
 * Set the maximum number of ways that can have the "EVIST_LAST" class.
 */
static void gk20a_ltc_set_max_ways_evict_last(struct gk20a *g, u32 max_ways)
{
	u32 mgmt_reg;

	mgmt_reg = gk20a_readl(g, ltc_ltcs_ltss_tstg_set_mgmt_r()) &
		~ltc_ltcs_ltss_tstg_set_mgmt_max_ways_evict_last_f(~0);
	mgmt_reg |= ltc_ltcs_ltss_tstg_set_mgmt_max_ways_evict_last_f(max_ways);

	gk20a_writel(g, ltc_ltcs_ltss_tstg_set_mgmt_r(), mgmt_reg);
}

/*
 * Sets the ZBC color for the passed index.
 */
static void gk20a_ltc_set_zbc_color_entry(struct gk20a *g,
					  struct zbc_entry *color_val,
					  u32 index)
{
	u32 i;
	u32 real_index = index + GK20A_STARTOF_ZBC_TABLE;

	gk20a_writel(g, ltc_ltcs_ltss_dstg_zbc_index_r(),
		     ltc_ltcs_ltss_dstg_zbc_index_address_f(real_index));

	for (i = 0;
	     i < ltc_ltcs_ltss_dstg_zbc_color_clear_value__size_1_v(); i++)
		gk20a_writel(g, ltc_ltcs_ltss_dstg_zbc_color_clear_value_r(i),
			     color_val->color_l2[i]);
}

/*
 * Sets the ZBC depth for the passed index.
 */
static void gk20a_ltc_set_zbc_depth_entry(struct gk20a *g,
					  struct zbc_entry *depth_val,
					  u32 index)
{
	u32 real_index = index + GK20A_STARTOF_ZBC_TABLE;

	gk20a_writel(g, ltc_ltcs_ltss_dstg_zbc_index_r(),
		     ltc_ltcs_ltss_dstg_zbc_index_address_f(real_index));

	gk20a_writel(g, ltc_ltcs_ltss_dstg_zbc_depth_clear_value_r(),
		     depth_val->depth);
}

static void gk20a_ltc_init_cbc(struct gk20a *g, struct gr_gk20a *gr)
{
	u32 compbit_base_post_divide;
	u64 compbit_base_post_multiply64;
	u64 compbit_store_base_iova =
		NV_MC_SMMU_VADDR_TRANSLATE(gr->compbit_store.base_iova);
	u64 compbit_base_post_divide64 = (compbit_store_base_iova >>
		ltc_ltcs_ltss_cbc_base_alignment_shift_v());

	do_div(compbit_base_post_divide64, gr->num_fbps);
	compbit_base_post_divide = u64_lo32(compbit_base_post_divide64);

	compbit_base_post_multiply64 = ((u64)compbit_base_post_divide *
		gr->num_fbps) << ltc_ltcs_ltss_cbc_base_alignment_shift_v();

	if (compbit_base_post_multiply64 < compbit_store_base_iova)
		compbit_base_post_divide++;

	gk20a_writel(g, ltc_ltcs_ltss_cbc_base_r(),
		compbit_base_post_divide);

	gk20a_dbg(gpu_dbg_info | gpu_dbg_map | gpu_dbg_pte,
		   "compbit base.pa: 0x%x,%08x cbc_base:0x%08x\n",
		   (u32)(compbit_store_base_iova >> 32),
		   (u32)(compbit_store_base_iova & 0xffffffff),
		   compbit_base_post_divide);
}

/* Flushes the compression bit cache as well as "data".
 * Note: the name here is a bit of a misnomer.  ELPG uses this
 * internally... but ELPG doesn't have to be on to do it manually.
 */
static void gk20a_mm_g_elpg_flush_locked(struct gk20a *g)
{
	u32 data;
	s32 retry = 100;

	gk20a_dbg_fn("");

	/* Make sure all previous writes are committed to the L2. There's no
	   guarantee that writes are to DRAM. This will be a sysmembar internal
	   to the L2. */
	gk20a_writel(g, ltc_ltcs_ltss_g_elpg_r(),
		     ltc_ltcs_ltss_g_elpg_flush_pending_f());
	do {
		data = gk20a_readl(g, ltc_ltc0_ltss_g_elpg_r());

		if (ltc_ltc0_ltss_g_elpg_flush_v(data) ==
		    ltc_ltc0_ltss_g_elpg_flush_pending_v()) {
			gk20a_dbg_info("g_elpg_flush 0x%x", data);
			retry--;
			usleep_range(20, 40);
		} else
			break;
	} while (retry >= 0 || !tegra_platform_is_silicon());

	if (retry < 0)
		gk20a_warn(dev_from_gk20a(g),
			    "g_elpg_flush too many retries");

}
