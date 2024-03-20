/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) Telechips Inc.
 */

#ifndef VIDEO_OUTPUT_H
#define VIDEO_OUTPUT_H

#include <stdint.h>
#include <limits.h>
#include "overlay.h"

enum video_output_windoe_priority {
	VOUT_WIND_BACKGROUND,
	VOUT_WIND_FOREGROUND,
	VOUT_WIND_PRIO_MAX,
};

struct video_output_buffer {
	int				posx;
	int				posy;
	unsigned int			width;
	unsigned int			height;
	unsigned int			format;
	unsigned int			addrs[3];
};

struct video_output {
	struct overlay			ovl;

	struct video_output_buffer	buf;

	unsigned int			ignore_ovp;
};

extern int video_output_open_device(struct video_output *dev);
extern int video_output_close_device(const struct video_output *dev);
extern int video_output_init_params(struct video_output *dev);
extern int video_output_preview_buffer(const struct video_output *dev, const struct video_output_buffer *buf);
extern int video_output_get_window_priority(const struct video_output *dev, int *priority);
extern int video_output_set_window_priority(const struct video_output *dev, int priority);
extern int video_output_disable_path(const struct video_output *dev);

#endif//VIDEO_OUTPUT_H
