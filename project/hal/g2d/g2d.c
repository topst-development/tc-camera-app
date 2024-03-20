// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Telechips Inc.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "log.h"
#include "pmap.h"
#include "g2d.h"

#define GRAPHIC_DEVICE		("/dev/g2d")
#define GRAPHIC_PMAP		("overlay_rot")

int32_t g2d_memory_allocate(struct graphic2d *dev,
	uint32_t width, uint32_t height, uint32_t depth)
{
	struct pmap	g2d_pmap;
	int32_t		ret	= 0;

	// get pmap info
	(void)memset((void *)&g2d_pmap, 0, sizeof(g2d_pmap)); 
	ret = pmap_get_info(GRAPHIC_PMAP, &g2d_pmap);
	if (ret < 0) {
		loge("pmap_get_info, ret: %d\n", ret);
		return ret;
	}
	logd("g2d - base: 0x%08x, size: 0x%08x, arange: 0x%08x - 0x%08x\n",
		g2d_pmap.base, g2d_pmap.size, g2d_pmap.base + g2d_pmap.size);

	dev->phy_addr = g2d_pmap.base + ((width * height * depth) * (uint32_t)dev->id);
	logd("%10s%d: 0x%08x\n", "g2d", dev->id, dev->phy_addr);

	return ret;
}

int32_t g2d_memory_deallocate(struct graphic2d *dev)
{
	int32_t		ret	= 0;

	if (dev->phy_addr == (uint32_t)0) {
		loge("addr(0x%08x) is wrong\n", dev->phy_addr);
		ret = -1;
	} else {
		dev->phy_addr = 0;
	}

	return ret;
}

uint32_t g2d_get_memory_address(struct graphic2d *dev)
{
	logd("%10s%d: 0x%08x\n", "g2d", dev->id, dev->phy_addr);
	return dev->phy_addr;
}

int32_t g2d_rotation_do(struct graphic2d *dev, G2D_COMMON_TYPE *grp_arg)
{
	struct pollfd	poll_event;
	int32_t		ret		= 0;

	if (dev->fd <= 0) {
		loge("device: '%s', fd: %d\n", GRAPHIC_DEVICE, dev->fd);
		goto G2D_ERROR;
	}

	if (grp_arg == NULL) {
		loge("grp_arg is NULL\n");
		goto G2D_ERROR;
	}

	ret = ioctl(dev->fd, TCC_GRP_COMMON_IOCTRL, grp_arg);
	if (ret < 0) {
		loge("TCC_GRP_COMMON_IOCTRL, ret: %d\n", ret);
		return -1;
	}

	logd("G2D-SRC:: %d x %d, %d,%d ~ %d x %d (rot: %d)\n",
		grp_arg->src_imgx, grp_arg->src_imgy,
		grp_arg->crop_offx, grp_arg->crop_offy,
		grp_arg->crop_imgx, grp_arg->crop_imgy, grp_arg->ch_mode);
	logd("G2D-Dest:: %d x %d\n", grp_arg->dst_imgx, grp_arg->dst_imgy);
	logd("0x%x-0x%x-0x%x => 0x%x-0x%x-0x%x\n",
		grp_arg->src0, grp_arg->src1, grp_arg->src2,
		grp_arg->tgt0, grp_arg->tgt1, grp_arg->tgt2);

	if (grp_arg->responsetype == (uint32_t)G2D_INTERRUPT) {
		(void)memset(&poll_event, 0, sizeof(poll_event));
		poll_event.fd		= dev->fd;
		poll_event.events	= POLLIN;
		ret = poll(&poll_event, 1, 400);
		if (ret < 0) {
			loge("POLLIN, ret: %d\n", ret);
			return -1;
		} else if (ret == 0) {
			loge("g2d poll timeout\n");
			goto G2D_ERROR;
		} else {
			if (((uint32_t)poll_event.revents &
			     (uint32_t)POLLERR) != 0U) {
				loge("g2d poll POLLERR\n");
				return -1;
			} else {
				// normal situcation
				// do nothing
			}
		}
	}
	return 0;

G2D_ERROR:
	return -1;
}

int32_t g2d_rotation(struct graphic2d *dev,
	uint32_t src_y, uint32_t src_u, uint32_t src_v,
	uint32_t dst_y, uint32_t dst_u, uint32_t dst_v,
	uint32_t width, uint32_t height,
	uint32_t crop_offx, uint32_t crop_offy,
	uint32_t crop_imgx, uint32_t crop_imgy,
	uint32_t angle)
{
	G2D_COMMON_TYPE	grp_arg;
	int32_t		ret	= 0;

	logd("src - y: 0x%08x, u: 0x%08x, v: 0x%08x\n", src_y, src_u, src_v);
	logd("dst - y: 0x%08x, u: 0x%08x, v: 0x%08x\n", dst_y, dst_u, dst_v);
	logd("width: %d, height: %d, angle: %u\n", width, height, angle);

	// clear structure
	(void)memset((void *)&grp_arg, 0, sizeof(grp_arg));

	grp_arg.responsetype		= (uint32_t)G2D_INTERRUPT;
	grp_arg.src0			= src_y;
	grp_arg.src1			= src_u;
	grp_arg.src2			= src_v;
	grp_arg.srcfm.format		= (uint32_t)GE_ARGB8888;
	grp_arg.src_imgx		= (uint32_t)width;
	grp_arg.src_imgy		= (uint32_t)height;
	grp_arg.crop_offx		= crop_offx;
	grp_arg.crop_offy		= crop_offy;
	grp_arg.crop_imgx		= crop_imgx;
	grp_arg.crop_imgy		= crop_imgy;
	grp_arg.tgt0			= dst_y;
	grp_arg.tgt1			= dst_u;
	grp_arg.tgt2			= dst_v;
	grp_arg.tgtfm.format		= grp_arg.srcfm.format;
	grp_arg.ch_mode			= angle;
	if ((grp_arg.ch_mode == (uint32_t)ROTATE_90) ||
	    (grp_arg.ch_mode == (uint32_t)ROTATE_270)) {
		grp_arg.dst_imgx	= grp_arg.crop_imgy;
		grp_arg.dst_imgy	= grp_arg.crop_imgx;
	} else {
		grp_arg.dst_imgx	= grp_arg.crop_imgx;
		grp_arg.dst_imgy	= grp_arg.crop_imgy;
	}

	ret = g2d_rotation_do(dev, &grp_arg);

	return ret;
}

int32_t g2d_is_available(struct graphic2d *dev)
{
	int32_t		ret = 0;

	if (dev->fd == 0) {
		logd("fd(%d) is wrong\n", dev->fd);
		ret = -1;
	}

	return ret;
}

int32_t g2d_open(struct graphic2d *dev)
{
	char		name[1024]	= "";
	int32_t		ret = 0;

	if (dev->fd != 0) {
		loge("fd(%d) is NOT NULL\n", dev->fd);
		ret = -1;
	} else {
		if (dev->id == 0) {
			/* coverity[misra_c_2012_rule_21_6_violation : FALSE] */
			sprintf(name, "%s", GRAPHIC_DEVICE);
		} else {
			/* coverity[misra_c_2012_rule_21_6_violation : FALSE] */
			sprintf(name, "%s%d", GRAPHIC_DEVICE, dev->id);
		}
		dev->fd = open(name, O_RDWR);
		if (dev->fd == 0) {
			loge("open(%s), ret: %d\n", name, dev->fd);
			ret = -1;
		}
	}

	return ret;
}

int32_t g2d_close(struct graphic2d *dev)
{
	int32_t		ret = 0;

	if (dev->fd == 0) {
		loge("fd is already NULL\n");
		ret = -1;
	} else {
		ret = close(dev->fd);
		dev->fd = 0;
	}

	return ret;
}

