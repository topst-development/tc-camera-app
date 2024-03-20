// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Telechips Inc.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <tcc_types.h>
#include <limits.h>
#include "basic_operation.h"
#include "log.h"
#include "v4l2.h"

/* coverity[misra_c_2012_rule_9_3_violation : FALSE] */
static struct video_format video_format_table[V4L2_PIXEL_FORMAT_MAX] = {
	/* RGB */
	[V4L2_PIXEL_FORMAT_RGB24] = {
		.name				= "rgb24",
		/* 'RBG3' 24 RGB-8-8-8 */
		.format[FORMAT_TYPE_V4L2]	= V4L2_PIX_FMT_RGB24,
		/* B1 31:24, R 23:16, G 15:8, B0 7:0 : RGB24bit */
		.format[FORMAT_TYPE_VIOC]	= (unsigned int)TCC_LCDC_IMG_FMT_RGB888_3,
		.color_depth			= 3,
		.planes				= 1,
	},
	[V4L2_PIXEL_FORMAT_RGB32] = {
		.name				= "rgb32",
		/* 'RGB4' 32 RGB-8-8-8-8 */
		.format[FORMAT_TYPE_V4L2]	= V4L2_PIX_FMT_RGB32,
		/* A 31:24, R 23:16, G 15:8, B 7:0 : RGB32bit */
		.format[FORMAT_TYPE_VIOC]	= (unsigned int)TCC_LCDC_IMG_FMT_RGB888,
		.color_depth			= 4,
		.planes				= 1,
	},

	/* sequential (YUV packed) */
	[V4L2_PIXEL_FORMAT_UYVY] = {
		.name				= "uyvy",
		/* 'UYVY' 16 YUV 4:2:2 */
		.format[FORMAT_TYPE_V4L2]	= V4L2_PIX_FMT_UYVY,
		/* LSB [Y|U|Y|V] MSB : YCbCr 4:2:2 sequential */
		.format[FORMAT_TYPE_VIOC]	= (unsigned int)TCC_LCDC_IMG_FMT_UYVY,
		.color_depth			= 2,
		.planes				= 1,
	},
	[V4L2_PIXEL_FORMAT_VYUY] = {
		.name				= "vyuy",
		/* 'VYUY' 16 YUV 4:2:2 */
		.format[FORMAT_TYPE_V4L2]	= V4L2_PIX_FMT_VYUY,
		/* LSB [Y|V|Y|U] MSB : YCbCr 4:2:2 sequential */
		.format[FORMAT_TYPE_VIOC]	= (unsigned int)TCC_LCDC_IMG_FMT_VYUY,
		.color_depth			= 2,
		.planes				= 1,
	},
	[V4L2_PIXEL_FORMAT_YUYV] = {
		.name				= "yuyv",
		/* 'YUYV' 16 YUV 4:2:2 */
		.format[FORMAT_TYPE_V4L2]	= V4L2_PIX_FMT_YUYV,
		/* LSB [Y|U|Y|V] MSB : YCbCr 4:2:2 sequential */
		.format[FORMAT_TYPE_VIOC]	= (unsigned int)TCC_LCDC_IMG_FMT_YUYV,
		.color_depth			= 2,
		.planes				= 1,
	},
	[V4L2_PIXEL_FORMAT_YVYU] = {
		.name				= "yvyu",
		/* 'YVYU' 16 YVU 4:2:2 */
		.format[FORMAT_TYPE_V4L2]	= V4L2_PIX_FMT_YVYU,
		/* LSB [Y|V|Y|U] MSB : YCbCr 4:2:2 sequential */
		.format[FORMAT_TYPE_VIOC]	= (unsigned int)TCC_LCDC_IMG_FMT_YVYU,
		.color_depth			= 2,
		.planes				= 1,
	},

	/* sepatated (Y, U, V planar) */
	[V4L2_PIXEL_FORMAT_YVU420] = {
		.name				= "yvu420",
		/* 'YV12' 12 YVU 4:2:0 */
		.format[FORMAT_TYPE_V4L2]	= V4L2_PIX_FMT_YVU420,
		/* YCbCr 4:2:0 separated */
		.format[FORMAT_TYPE_VIOC]	= (unsigned int)TCC_LCDC_IMG_FMT_YUV420SP,
		.color_depth			= 2,
		.planes				= 3,
	},
	[V4L2_PIXEL_FORMAT_YUV420] = {
		.name				= "yuv420",
		/* 'YU12' 12 YUV 4:2:0 */
		.format[FORMAT_TYPE_V4L2]	= V4L2_PIX_FMT_YUV420,
		/* YCbCr 4:2:0 separated */
		.format[FORMAT_TYPE_VIOC]	= (unsigned int)TCC_LCDC_IMG_FMT_YUV420SP,
		.color_depth			= 2,
		.planes				= 3,
	},
	[V4L2_PIXEL_FORMAT_YUV422P] = {
		.name				= "yvu422p",
		/* '422P' 16 YVU422 Planar */
		.format[FORMAT_TYPE_V4L2]	= V4L2_PIX_FMT_YUV422P,
		/* YCbCr 4:2:2 separated */
		.format[FORMAT_TYPE_VIOC]	= (unsigned int)TCC_LCDC_IMG_FMT_YUV422SP,
		.color_depth			= 2,
		.planes				= 3,
	},

	/* interleaved (Y palnar, UV planar) */
	[V4L2_PIXEL_FORMAT_NV12] = {
		.name				= "nv12",
		/* 'NV12' 12 Y/CbCr 4:2:0 */
		.format[FORMAT_TYPE_V4L2]	= V4L2_PIX_FMT_NV12,
		/* YCbCr 4:2:0 interleaved type0 */
		.format[FORMAT_TYPE_VIOC]	= (unsigned int)TCC_LCDC_IMG_FMT_YUV420ITL0,
		.color_depth			= 2,
		.planes				= 2,
	},
	[V4L2_PIXEL_FORMAT_NV21] = {
		.name				= "nv21",
		/* 'NV21' 12 Y/CrCb 4:2:0 */
		.format[FORMAT_TYPE_V4L2]	= V4L2_PIX_FMT_NV21,
		/* YCbCr 4:2:0 interleaved type1 */
		.format[FORMAT_TYPE_VIOC]	= (unsigned int)TCC_LCDC_IMG_FMT_YUV420ITL1,
		.color_depth			= 2,
		.planes				= 2,
	},
	[V4L2_PIXEL_FORMAT_NV16] = {
		.name				= "nv16",
		/* 'NV16' 16 Y/CbCr 4:2:2 */
		.format[FORMAT_TYPE_V4L2]	= V4L2_PIX_FMT_NV16,
		/* YCbCr 4:2:2 interleaved type0 */
		.format[FORMAT_TYPE_VIOC]	= (unsigned int)TCC_LCDC_IMG_FMT_YUV422ITL0,
		.color_depth			= 2,
		.planes				= 2,
	},
	[V4L2_PIXEL_FORMAT_NV61] = {
		.name				= "nv61",
		/* 'NV61' 16 Y/CrCb 4:2:2 */
		.format[FORMAT_TYPE_V4L2]	= V4L2_PIX_FMT_NV61,
		/* YCbCr 4:2:2 interleaved type1 */
		.format[FORMAT_TYPE_VIOC]	= (unsigned int)TCC_LCDC_IMG_FMT_YUV422ITL1,
		.color_depth			= 2,
		.planes				= 2,
	}
};

unsigned int v4l2_get_v4l2_format_by_name(const char *name)
{
	unsigned int	nEntry		=
		u64_to_u32(sizeof(video_format_table) / sizeof(video_format_table[0]));
	unsigned int	idxEntry	= 0;
	unsigned int	type		= (unsigned int)FORMAT_TYPE_V4L2;
	unsigned int	ret		= 0;
	unsigned int	is_found	= 0;

	for (/*idxEntry = 0*/; idxEntry < nEntry; idxEntry++) {
		if (strcmp(name, video_format_table[idxEntry].name) == 0) {
			ret = video_format_table[idxEntry].format[type];
			logd("name: %s, v4l2 format: 0x%08x\n", name, ret);
			is_found = 1;
			break;
		}
	}

	return (is_found == 1U) ? ret : (unsigned int)V4L2_PIXEL_FORMAT_MAX;
}

const char *v4l2_get_format_name_by_v4l2_format(unsigned int format)
{
	unsigned int	nEntry		=
		u64_to_s32(sizeof(video_format_table) / sizeof(video_format_table[0]));
	unsigned int	idxEntry	= 0;
	unsigned int	type		= (unsigned int)FORMAT_TYPE_V4L2;
	const char	*ret		= NULL;

	for (/*idxEntry = 0*/; idxEntry < nEntry; idxEntry++) {
		if (video_format_table[idxEntry].format[type] == format) {
			ret = video_format_table[idxEntry].name;
			logd("v4l2 format: 0x%08x, name: %s\n", format, ret);
			break;
		}
	}

	return ret;
}

unsigned int v4l2_convert_format_from_v4l2_to_vioc(unsigned int format)
{
	unsigned int	nEntry		=
		u64_to_u32(sizeof(video_format_table) / sizeof(video_format_table[0]));
	unsigned int	idxEntry	= 0;
	unsigned int	type_from	= (unsigned int)FORMAT_TYPE_V4L2;
	unsigned int	type_to		= (unsigned int)FORMAT_TYPE_VIOC;
	unsigned int	ret		= 0;

	for (/*idxEntry = 0*/; idxEntry < nEntry; idxEntry++) {
		if (video_format_table[idxEntry].format[type_from] == format) {
			ret = video_format_table[idxEntry].format[type_to];
			logd("v4l2: 0x%08x -> vioc: 0x%08x\n", format, ret);
			break;
		}
	}

	return ret;
}

unsigned int v4l2_get_color_depth_by_v4l2_format(unsigned int format)
{
	unsigned int	nEntry		=
		u64_to_s32(sizeof(video_format_table) / sizeof(video_format_table[0]));
	unsigned int	idxEntry	= 0;
	unsigned int	type		= (unsigned int)FORMAT_TYPE_V4L2;
	unsigned int	color_depth	= 0;

	for (/*idxEntry = 0*/; idxEntry < nEntry; idxEntry++) {
		if (video_format_table[idxEntry].format[type] == format) {
			color_depth = video_format_table[idxEntry].color_depth;
			logd("v4l2 format: 0x%08x, color depth: %u\n",
				format, color_depth);
			break;
		}
	}

	return color_depth;
}

unsigned int v4l2_get_planes_by_v4l2_format(unsigned int format)
{
	unsigned int	nEntry		=
		u64_to_u32(sizeof(video_format_table) / sizeof(video_format_table[0]));
	unsigned int	idxEntry	= 0;
	unsigned int	type		= (unsigned int)FORMAT_TYPE_V4L2;
	unsigned int	planes		= 0;

	for (/*idxEntry = 0*/; idxEntry < nEntry; idxEntry++) {
		if (video_format_table[idxEntry].format[type] == format) {
			planes = video_format_table[idxEntry].planes;
			logd("v4l2 format: 0x%08x, planes: %u\n",
				format, planes);
			break;
		}
	}

	return planes;
}

unsigned int v4l2_get_v4l2_field(unsigned int format)
{
	unsigned int	field		= 0;

	switch (format) {
	case V4L2_PIX_FMT_RGB24:
	case V4L2_PIX_FMT_RGB32:
		/* V4L2_FIELD_ANY */
		break;
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_YUV420:
	case V4L2_PIX_FMT_YVU420:
	case V4L2_PIX_FMT_YUV422P:
		field = (unsigned int)V4L2_FIELD_INTERLACED;
		break;
	default:
		loge("v4l2 format (0x%08x) is not supported\n", format);
		break;
	}

	return field;
}

unsigned int v4l2_get_v4l2_sizeimage(unsigned int format, unsigned int width, unsigned int height)
{
	unsigned int		size	= 0;

	switch (format) {
	case V4L2_PIX_FMT_RGB24:
		/* coverity[misra_c_2012_rule_10_7_violation : FALSE] */
		size = u64_to_u32(width * height * 3UL);
		break;
	case V4L2_PIX_FMT_RGB32:
		size = u64_to_u32(width * height * 4UL);
		break;
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_YUV422P:
		size = u64_to_u32(width * height * 2UL);
		break;
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_YUV420:
	case V4L2_PIX_FMT_YVU420:
		size = u64_to_u32((width * height * 3UL) / 2UL);
		break;
	default:
		loge("v4l2 format (0x%08x) is not supported\n", format);
		break;
	}

	return size;
}
