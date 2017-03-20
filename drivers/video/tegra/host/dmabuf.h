/*
 * drivers/video/tegra/host/dmabuf.h
 *
 * Tegra Graphics Host dmabuf memory manager
 *
 * Copyright (c) 2012-2014, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef __NVHOST_DMABUF_H
#define __NVHOST_DMABUF_H

#include "nvhost_memmgr.h"

struct nvhost_chip_support;
struct platform_device;

struct mem_mgr *nvhost_dmabuf_alloc_mgr(void);
void nvhost_dmabuf_put_mgr(struct mem_mgr *mgr);
struct mem_mgr *nvhost_dmabuf_get_mgr(struct mem_mgr *mgr);
struct mem_mgr *nvhost_dmabuf_get_mgr_file(int fd);
struct mem_handle *nvhost_dmabuf_alloc(struct mem_mgr *mgr,
		size_t size, size_t align, int flags);
void nvhost_dmabuf_put(struct mem_handle *handle);
struct sg_table *nvhost_dmabuf_pin(struct mem_handle *handle);
void nvhost_dmabuf_unpin(struct mem_handle *handle, struct sg_table *sgt);
void *nvhost_dmabuf_mmap(struct mem_handle *handle);
void nvhost_dmabuf_munmap(struct mem_handle *handle, void *addr);
void *nvhost_dmabuf_kmap(struct mem_handle *handle, unsigned int pagenum);
void nvhost_dmabuf_kunmap(struct mem_handle *handle, unsigned int pagenum,
		void *addr);
struct mem_handle *nvhost_dmabuf_get(ulong id, struct platform_device *dev);
int nvhost_dmabuf_get_param(struct mem_mgr *memmgr, struct mem_handle *handle,
			   u32 param, u64 *result);
size_t nvhost_dmabuf_size(struct mem_handle *handle);
#endif
