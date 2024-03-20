/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) Telechips Inc.
 */

#ifndef V4L2_H
#define V4L2_H

#include <stdint.h>

extern unsigned int v4l2_get_v4l2_format_by_name(const char *name);
extern const char *v4l2_get_format_name_by_v4l2_format(unsigned int format);
extern unsigned int v4l2_convert_format_from_v4l2_to_vioc(unsigned int format);
extern unsigned int v4l2_get_color_depth_by_v4l2_format(unsigned int format);
extern unsigned int v4l2_get_planes_by_v4l2_format(unsigned int format);
extern unsigned int v4l2_get_v4l2_field(unsigned int format);
extern unsigned int v4l2_get_v4l2_sizeimage(unsigned int format, unsigned int width, unsigned int height);

enum v4l2_format_conversion_type {
	FORMAT_TYPE_V4L2,
	FORMAT_TYPE_VIOC,
	FORMAT_TYPE_MAX,
};

struct video_format {
	const char	*name;
	unsigned int	format[FORMAT_TYPE_MAX];
	unsigned int	color_depth;
	unsigned int	planes;
};

enum color_format {
	V4L2_PIXEL_FORMAT_RGB24,
	V4L2_PIXEL_FORMAT_RGB32,
	V4L2_PIXEL_FORMAT_UYVY,
	V4L2_PIXEL_FORMAT_VYUY,
	V4L2_PIXEL_FORMAT_YUYV,
	V4L2_PIXEL_FORMAT_YVYU,
	V4L2_PIXEL_FORMAT_YVU420,
	V4L2_PIXEL_FORMAT_YUV420,
	V4L2_PIXEL_FORMAT_YUV422P,
	V4L2_PIXEL_FORMAT_NV12,
	V4L2_PIXEL_FORMAT_NV21,
	V4L2_PIXEL_FORMAT_NV16,
	V4L2_PIXEL_FORMAT_NV61,
	V4L2_PIXEL_FORMAT_MAX,
};

#endif//V4L2_H
