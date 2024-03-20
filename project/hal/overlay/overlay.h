/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) Telechips Inc.
 */

#ifndef OVERLAY_H
#define OVERLAY_H

#include <stdint.h>
#include "tcc_overlay_ioctl.h"

struct overlay {
	int			id;
	int			fd;
	overlay_config_t	cfg;

	unsigned int		rdma_layer;
	int			wmix_fovp;
	int			wmix_bovp;
};

extern int overlay_open_device(struct overlay *dev);
extern int overlay_close_device(const struct overlay *dev);
extern int overlay_get_default_ovps(struct overlay *dev);
extern int overlay_qbuf(const struct overlay *dev, overlay_video_buffer_t *buf);
extern int overlay_get_ovp(const struct overlay *dev, int *ovp);
extern int overlay_set_ovp(const struct overlay *dev, int ovp);
extern int overlay_disable_layer(const struct overlay *dev);

#endif//OVERLAY_H
