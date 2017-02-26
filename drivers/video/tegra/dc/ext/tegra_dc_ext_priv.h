/*
 * drivers/video/tegra/dc/ext/tegra_dc_ext_priv.h
 *
 * Copyright (c) 2011-2014, NVIDIA CORPORATION, All rights reserved.
 *
 * Author: Robert Morell <rmorell@nvidia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#ifndef __TEGRA_DC_EXT_PRIV_H
#define __TEGRA_DC_EXT_PRIV_H

#include <linux/cdev.h>
#include <linux/dma-buf.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/poll.h>

#include <mach/dc.h>

#include <video/tegra_dc_ext.h>

struct tegra_dc_ext;

struct tegra_dc_ext_user {
	struct tegra_dc_ext	*ext;
};

struct tegra_dc_dmabuf {
	struct dma_buf *buf;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
};

enum {
	TEGRA_DC_Y,
	TEGRA_DC_U,
	TEGRA_DC_V,
	TEGRA_DC_NUM_PLANES,
};

struct tegra_dc_ext_win {
	struct tegra_dc_ext	*ext;

	int			idx;

	struct tegra_dc_ext_user *user;

	struct mutex		lock;

	/* Current dmabuf (if any) for Y, U, V planes */
	struct tegra_dc_dmabuf	*cur_handle[TEGRA_DC_NUM_PLANES];

	struct workqueue_struct	*flip_wq;

	atomic_t		nr_pending_flips;

	struct mutex		queue_lock;

	struct list_head	timestamp_queue;

	bool			enabled;
};

struct tegra_dc_ext {
	struct tegra_dc			*dc;

	struct cdev			cdev;
	struct device			*dev;

	struct tegra_dc_ext_win		win[DC_N_WINDOWS];

	struct {
		struct tegra_dc_ext_user	*user;
		struct tegra_dc_dmabuf		*cur_handle;
		struct mutex			lock;
	} cursor;

	bool				enabled;
	bool				vblank_enabled;
};

#define TEGRA_DC_EXT_EVENT_MASK_ALL		\
	(TEGRA_DC_EXT_EVENT_HOTPLUG |		\
	 TEGRA_DC_EXT_EVENT_VBLANK |		\
	 TEGRA_DC_EXT_EVENT_BANDWIDTH_INC |	\
	 TEGRA_DC_EXT_EVENT_BANDWIDTH_DEC)

#define TEGRA_DC_EXT_EVENT_MAX_SZ	16

struct tegra_dc_ext_event_list {
	struct tegra_dc_ext_event	event;
	/* The data field _must_ follow the event field. */
	char				data[TEGRA_DC_EXT_EVENT_MAX_SZ];

	struct list_head		list;
};

struct tegra_dc_ext_control_user {
	struct tegra_dc_ext_control	*control;

	struct list_head		event_list;
	atomic_t			num_events;

	u32				event_mask;

	struct tegra_dc_ext_event_list	event_to_copy;
	loff_t				partial_copy;

	struct mutex			lock;

	struct list_head		list;
};

struct tegra_dc_ext_control {
	struct cdev			cdev;
	struct device			*dev;

	struct list_head		users;

	struct mutex			lock;
};

extern int tegra_dc_ext_devno;
extern struct class *tegra_dc_ext_class;

extern int tegra_dc_ext_pin_window(struct tegra_dc_ext_user *user, u32 id,
				   struct tegra_dc_dmabuf **handle,
				   dma_addr_t *phys_addr);

extern int tegra_dc_ext_get_cursor(struct tegra_dc_ext_user *user);
extern int tegra_dc_ext_put_cursor(struct tegra_dc_ext_user *user);
extern int tegra_dc_ext_set_cursor_image(struct tegra_dc_ext_user *user,
					 struct tegra_dc_ext_cursor_image *);
extern int tegra_dc_ext_set_cursor(struct tegra_dc_ext_user *user,
				   struct tegra_dc_ext_cursor *);
extern int tegra_dc_ext_cursor_clip(struct tegra_dc_ext_user *user,
					int *args);

extern int tegra_dc_ext_set_cursor_image_low_latency(
		struct tegra_dc_ext_user *user,
		struct tegra_dc_ext_cursor_image *);

extern int tegra_dc_ext_set_cursor_low_latency(struct tegra_dc_ext_user *user,
					struct tegra_dc_ext_cursor_image *);

extern int tegra_dc_ext_control_init(void);

extern int tegra_dc_ext_queue_hotplug(struct tegra_dc_ext_control *,
				      int output);
extern int tegra_dc_ext_queue_vblank(struct tegra_dc_ext_control *,
				      int output, ktime_t timestamp);
extern int tegra_dc_ext_queue_bandwidth_renegotiate(
			struct tegra_dc_ext_control *, int output,
			struct tegra_dc_bw_data *data);
extern ssize_t tegra_dc_ext_event_read(struct file *filp, char __user *buf,
				       size_t size, loff_t *ppos);
extern unsigned int tegra_dc_ext_event_poll(struct file *, poll_table *);

extern int tegra_dc_ext_get_num_outputs(void);

#endif /* __TEGRA_DC_EXT_PRIV_H */
