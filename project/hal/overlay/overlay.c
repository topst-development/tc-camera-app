// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Telechips Inc.
 */

#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <limits.h>

#include <video/tcc/tcc_cam_ioctrl.h>

#include "log.h"
#include "v4l2.h"
#include "overlay.h"
#include "basic_operation.h"

#define DEVICE_NAME			("/dev/overlay")

#define NUM_OF_OVPS			(30U)

int overlay_open_device(struct overlay *dev)
{
	char				name[1024]	= "";
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is null\n");
		ret = -1;
	} else if (dev->fd != 0) {
		loge("fd(%d) is wrong\n", dev->fd);
		ret = -1;
	} else {
		if (dev->id == 0) {
			/* coverity[misra_c_2012_rule_21_6_violation : FALSE] */
			ret = sprintf(name, "%s", DEVICE_NAME);
		} else {
			/* coverity[misra_c_2012_rule_21_6_violation : FALSE] */
			ret = sprintf(name, "%s%d", DEVICE_NAME, dev->id);
		}
		if (ret < 0) {
			/* error */
			loge("Failed on sprintf\n");
		} else {
			logd("device name: %s\n", name);
			dev->fd = open(name, O_RDWR);
			if (dev->fd < 0) {
				loge("fd(%d) is wrong", dev->fd);
				ret = -1;
			}
		}
	}

	return ret;
}

int overlay_close_device(const struct overlay *dev)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else if (dev->fd <= 0) {
		loge("fd(%d) is wrong\n", dev->fd);
		ret = -1;
	} else {
		/* close */
		ret = close(dev->fd);
	}

	return ret;
}

int overlay_get_default_ovps(struct overlay *dev)
{
	const unsigned int		ovp_list[NUM_OF_OVPS] = {
		[0]  = 0x3012,
		[1]  = 0x3102,
		[2]  = 0x3021,
		[3]  = 0x3201,
		[4]  = 0x3120,
		[5]  = 0x3210,

		[8]  = 0x2013,
		[9]  = 0x2103,
		[10] = 0x2031,
		[11] = 0x2301,
		[12] = 0x2130,
		[13] = 0x2310,

		[16] = 0x1023,
		[17] = 0x1203,
		[18] = 0x1203,
		[19] = 0x1302,
		[20] = 0x1230,
		[21] = 0x1320,

		[24] = 0x0123,
		[25] = 0x0213,
		[26] = 0x0132,
		[27] = 0x0312,
		[28] = 0x0231,
		[29] = 0x0321,
	};
	unsigned int			idxLoop		= 0;
	unsigned int			nLoop		= 4;
	unsigned int			rdma_layer	= 0;
	unsigned int			idxLayer	= 0;
	unsigned int			prio_value	= 0;
	unsigned int			idxOvp		= 0;
	unsigned int			nOvp		= NUM_OF_OVPS;
	int				ret		= 0;

	if (dev->fd <= 0) {
		loge("fd(%d) is wrong\n", dev->fd);
		ret = -1;
	} else {
		ret = ioctl(dev->fd, OVERLAY_GET_LAYER, &dev->rdma_layer);
		if (ret < 0) {
			loge("ret: %d\n", ret);
			ret = -1;
		} else {
			logd("rdma_layer: %u\n", dev->rdma_layer);

			rdma_layer = dev->rdma_layer % 4U;

			/* set top priority */
			prio_value |= (rdma_layer << (4U * (nLoop - 1U - idxLoop)));

			for (idxLoop = 0; idxLoop < nLoop; idxLoop++) {
				if (idxLoop == rdma_layer) {
					/* just increase the index of the layer */
					idxLayer++;
				} else {
					prio_value |= (idxLayer << (4U * (nLoop - 1U - idxLoop)));
					idxLayer++;
					logd("layer: %u, prio_value: 0x%08x, idxLayer: %u\n", idxLoop, prio_value, idxLayer);
				}
			}

			for (idxOvp = 0; idxOvp < nOvp; idxOvp++) {
				if (prio_value == ovp_list[idxOvp]) {
					/* break to return the current ovp */
					break;
				}
			}

			dev->wmix_bovp = 24;
			dev->wmix_fovp = u32_to_s32(idxOvp);
			logd("bovp: %d, fovp: %d\n", dev->wmix_bovp, dev->wmix_fovp);
		}
	}

	return ret;
}

int overlay_qbuf(const struct overlay *dev, overlay_video_buffer_t *buf)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else if (buf == NULL) {
		loge("buf is NULL\n");
		ret = -1;
	} else {
		ret = ioctl(dev->fd, OVERLAY_PUSH_VIDEO_BUFFER, buf);
		if (ret < 0) {
			loge("ret: %d\n", ret);
			ret = -1;
		}
	}

	return ret;
}

int overlay_get_ovp(const struct overlay *dev, int *ovp)
{
	unsigned int			arg		= 0;
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		ret = ioctl(dev->fd, OVERLAY_GET_OVP, &arg);
		if (ret < 0) {
			loge("ret: %d\n", ret);
			ret = -1;
		} else {
			logd("RDMA: %d, WMIX OVP: %d\n", dev->rdma_layer, arg);
			*ovp = u32_to_s32(arg);
		}
	}

	return ret;
}

int overlay_set_ovp(const struct overlay *dev, int ovp)
{
	unsigned int			arg		= 0;
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		arg = s32_to_u32(ovp);
		ret = ioctl(dev->fd, OVERLAY_SET_OVP, &arg);
		if (ret < 0) {
			loge("ret: %d\n", ret);
			ret = -1;
		}
	}

	return ret;
}

int overlay_disable_layer(const struct overlay *dev)
{
	int				arg		= 0;
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		ret = ioctl(dev->fd, OVERLAY_DISALBE_LAYER, &arg);
		if (ret < 0) {
			loge("ret: %d\n", ret);
			ret = -1;
		}
	}

	return ret;
}
