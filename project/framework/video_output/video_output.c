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
#include <video/tcc/tcc_cam_ioctrl.h>
#include <limits.h>
#include "log.h"
#include "v4l2.h"
#include "video_output.h"
#include "basic_operation.h"

int video_output_open_device(struct video_output *dev)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		ret = overlay_open_device(&dev->ovl);
		if (ret < 0) {
			loge("overlay_open_device, ret: %d\n", ret);
			ret = -1;
		}
	}

	return ret;
}

int video_output_close_device(const struct video_output *dev)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		ret = overlay_close_device(&dev->ovl);
		if (ret < 0) {
			loge("overlay_close_device, ret: %d\n", ret);
			ret = -1;
		}
	}

	return ret;
}

int video_output_init_params(struct video_output *dev)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		if ((dev->ovl.wmix_fovp == -1) ||
		    (dev->ovl.wmix_bovp == -1)) {
			ret = overlay_get_default_ovps(&dev->ovl);
			if (ret < 0) {
				loge("overlay_get_default_ovps, ret: %d\n", ret);
				ret = -1;
			}
		}
	}

	return ret;
}

int video_output_preview_buffer(const struct video_output *dev,
	const struct video_output_buffer *buf)
{
	overlay_video_buffer_t		overlay_buf	= { 0, };
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else if (buf == NULL) {
		loge("buf is NULL\n");
		ret = -1;
	} else {
		logd("addrs: 0x%08x, 0x%08x, 0x%08x\n",
		     buf->addrs[0], buf->addrs[1], buf->addrs[2]);

		(void)memset((void *)&overlay_buf, 0, sizeof(overlay_buf));

		overlay_buf.cfg.sx	= s32_to_u32(buf->posx);
		overlay_buf.cfg.sy	= s32_to_u32(buf->posy);
		overlay_buf.cfg.width	= buf->width;
		overlay_buf.cfg.height	= buf->height;
		overlay_buf.cfg.format	= v4l2_convert_format_from_v4l2_to_vioc(buf->format);
		overlay_buf.addr	= buf->addrs[0];
		overlay_buf.addr1	= buf->addrs[1];
		overlay_buf.addr2	= buf->addrs[2];

		ret = overlay_qbuf(&dev->ovl, &overlay_buf);
		if (ret < 0) {
			loge("overlay_qbuf, ret: %d\n", ret);
			ret = -1;
		}
	}

	return ret;
}

int video_output_get_window_priority(const struct video_output *dev, int *priority)
{
	int				ovp		= 0;
	int				ret		= 0;

	if (dev == NULL) {
		/* coverity [misra_c_2012_rule_21_6_violation : FALSE] */
		loge("dev is NULL\n");
		ret = -1;
	} else {
		ret = overlay_get_ovp(&dev->ovl, &ovp);
		if (ret < 0) {
			/* coverity [misra_c_2012_rule_21_6_violation : FALSE] */
			loge("overlay_get_ovp, ret: %d\n", ret);
			ret = -1;
		}
	}

	if (ret != -1) {
		if (ovp == dev->ovl.wmix_bovp) {
			/* bottom */
			*priority = (int)VOUT_WIND_BACKGROUND;
		} else if (ovp == dev->ovl.wmix_fovp) {
			/* top */
			*priority = (int)VOUT_WIND_FOREGROUND;
		} else {
			/* coverity [misra_c_2012_rule_21_6_violation : FALSE] */
			loge("ovp(%d) is wrong\n", ovp);
			*priority = -1;
			ret = -1;
		}

		/* coverity [misra_c_2012_rule_21_6_violation : FALSE] */
		logi("rdma: %d, ovp: %d, priority: %s\n",
			dev->ovl.rdma_layer, ovp,
			(*priority == (int)VOUT_WIND_FOREGROUND) ? "FOREGROUND" : "BACKGROUND");
	}

	return ret;
}

int video_output_set_window_priority(const struct video_output *dev, int priority)
{
	int				ovp		= 0;
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		switch (priority) {
		case (int)VOUT_WIND_BACKGROUND:
			ovp = dev->ovl.wmix_bovp;
			break;
		case (int)VOUT_WIND_FOREGROUND:
			ovp = dev->ovl.wmix_fovp;
			break;
		default:
			loge("priority(%d) is wrong\n", priority);
			ret = -1;
			break;
		}

		if (ret != -1) {
			ret = overlay_set_ovp(&dev->ovl, ovp);
			if (ret < 0) {
				loge("overlay_set_ovp, ret: %d\n", ret);
				ret = -1;
			}
		}
	}

	return ret;
}

int video_output_disable_path(const struct video_output *dev)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		ret = overlay_disable_layer(&dev->ovl);
		if (ret < 0) {
			loge("overlay_disable_layer, ret: %d\n", ret);
			ret = -1;
		}
	}

	return ret;
}
