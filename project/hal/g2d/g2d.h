/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) Telechips Inc.
 */

#ifndef G2D_H
#define G2D_H

#include <stdint.h>
#include "tcc_grp_ioctrl.h"

struct graphic2d {
	int32_t			id;
	int32_t			fd;

	uint32_t		phy_addr;
};

extern int32_t g2d_memory_allocate(struct graphic2d *dev,
	uint32_t width, uint32_t height, uint32_t depth);
extern int32_t g2d_memory_deallocate(struct graphic2d *dev);
extern uint32_t g2d_get_memory_address(struct graphic2d *dev);
extern int32_t g2d_rotation(struct graphic2d *dev,
	uint32_t src_y, uint32_t src_u, uint32_t src_v,
	uint32_t dst_y, uint32_t dst_u, uint32_t dst_v,
	uint32_t width, uint32_t height,
	uint32_t crop_offx, uint32_t crop_offy,
	uint32_t crop_imgx, uint32_t crop_imgy,
	uint32_t angle);
extern int32_t g2d_is_available(struct graphic2d *dev);
extern int32_t g2d_open(struct graphic2d *dev);
extern int32_t g2d_close(struct graphic2d *dev);

#endif//G2D_H

