// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Telechips Inc.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <limits.h>
#include "log.h"
#include "v4l2_capture.h"
#include "basic_operation.h"

#define DEVICE_NAME			("/dev/video")

int v4l2_capture_open_device(struct v4l2_capture *dev)
{
	char				name[1024]	= "";
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_21_6_violation : FALSE] */
		ret = sprintf(name, "%s%d", DEVICE_NAME, dev->id);
		if (ret < 0) {
			loge("sprintf, ret: %d\n", ret);
			ret = -1;
		} else {
			logd("device name: %s\n", name);
			dev->fd = open(name, O_RDWR);
			if (dev->fd < 0) {
				loge("open(%s), fd: %d\n", name, dev->fd);
				ret = -1;
			}
		}
	}

	return ret;
}

int v4l2_capture_close_device(const struct v4l2_capture *dev)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		if (dev->fd <= 0) {
			loge("fd(%d) is wrong\n", dev->fd);
			ret = -1;
		} else {
			/* close */
			ret = close(dev->fd);
		}
	}

	return ret;
}

int v4l2_capture_query_capabilities(const struct v4l2_capture *dev,
	struct v4l2_capability *cap)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_QUERYCAP, cap);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_set_format(const struct v4l2_capture *dev, struct v4l2_format *fmt)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_S_FMT, fmt);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_reqbufs(const struct v4l2_capture *dev,
	struct v4l2_requestbuffers *req)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_REQBUFS, req);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_querybuf(const struct v4l2_capture *dev,
	struct v4l2_buffer *buf)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_QUERYBUF, buf);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_qbuf(const struct v4l2_capture *dev,
	struct v4l2_buffer *buf)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_QBUF, buf);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_expbuf(const struct v4l2_capture *dev,
	struct v4l2_exportbuffer *buf)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_EXPBUF, buf);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_dqbuf(const struct v4l2_capture *dev, struct v4l2_buffer *buf)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_DQBUF, buf);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_streamon(const struct v4l2_capture *dev, int *arg)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_STREAMON, arg);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_streamoff(const struct v4l2_capture *dev, int *arg)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_STREAMOFF, arg);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_enuminput(const struct v4l2_capture *dev,
	struct v4l2_input *input)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_ENUMINPUT, input);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_g_input(const struct v4l2_capture *dev, int *index)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_G_INPUT, index);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_s_selection(const struct v4l2_capture *dev,
	struct v4l2_selection *sel)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_S_SELECTION, sel);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_g_parm(const struct v4l2_capture *dev,
	struct v4l2_streamparm *parm)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_G_PARM, parm);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_s_parm(const struct v4l2_capture *dev,
	struct v4l2_streamparm *parm)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_S_PARM, parm);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_enum_framesize(const struct v4l2_capture *dev,
	struct v4l2_frmsizeenum *frmsizeenum)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_ENUM_FRAMESIZES, frmsizeenum);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_enum_frameintervals(const struct v4l2_capture *dev,
	struct v4l2_frmivalenum *frmivalenum)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_ENUM_FRAMEINTERVALS, frmivalenum);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_check_v4l2_dev_status(const struct v4l2_capture *dev,
	int *status)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_CHECK_PATH_STATUS, status);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_g_lastframe_addrs(const struct v4l2_capture *dev,
	unsigned int *addrs)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_G_LASTFRAME_ADDRS, addrs);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_create_lastframe(const struct v4l2_capture *dev,
	unsigned int *addrs)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_CREATE_LASTFRAME, addrs);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_s_handover(const struct v4l2_capture *dev,
	int *handover)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_S_HANDOVER, handover);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_s_lut(const struct v4l2_capture *dev,
	struct vin_lut *lut)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
		ret = ioctl(dev->fd, VIDIOC_S_LUT, lut);
		if (ret < 0) {
			/* error */
			loge("Failed on ioctl: %s\n", strerror(errno));
		}
	}

	return ret;
}

int v4l2_capture_poll(const struct v4l2_capture *dev, int event, int timeout)
{
	struct pollfd			pfd;
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		/* set pfd */
		(void)memset((void *)&pfd, 0, sizeof(pfd));
		pfd.fd		= dev->fd;
		pfd.events	= u32_to_s16(event);

		ret = poll(&pfd, 1, timeout);
		logd("poll, ret: %d, revents: %d\n", ret, pfd.revents);
		if (ret > 0) {
			/* return revents */
			ret = (int)pfd.revents;
		}
	}

	return ret;
}
