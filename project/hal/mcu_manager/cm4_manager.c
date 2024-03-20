// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Telechips Inc.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "log.h"
#include "cm4_manager.h"

#define CM_DEVICE_NAME			("/dev/tcc_cm_ctrl")

int cm4_manager_open_device(struct cm4_manager *dev)
{
	int		ret	= 0;

	// open cm4 manager driver
	dev->fd = open(CM_DEVICE_NAME, O_RDWR);
	if (dev->fd < 0) {
		loge("cm4_manager_open_device, fd: %d\n", dev->fd);
		ret = -1;
	}

	return ret;
}

int cm4_manager_close_device(struct cm4_manager *dev)
{
	int		ret	= 0;

	// close the device
	if (dev->fd <= 0) {
		loge("cm4_manager_close_device, fd: %d\n", dev->fd);
		ret = -1;
	} else {
		ret = close(dev->fd);
	}

	return ret;
}

int cm4_manager_status_cm_control(struct cm4_manager *dev)
{
	int		cmd	= CM_CTRL_IOCTL_KNOCK;
	unsigned long	arg	= 0;
	int		ret	= 0;

	ret = ioctl(dev->fd, cmd, &arg);
	if (ret < 0) {
		loge("cm_control cmd: 0x%08x, ret: %d\n", cmd, ret);
		return -1;
	}

	return ret;
}

int cm4_manager_disable_cm_control(struct cm4_manager *dev, unsigned long arg)
{
	int		ret	= 0;

	ret = ioctl(dev->fd, CM_CTRL_IOCTL_CMD, &arg);
	if (ret < 0) {
		loge("cm_control cmd: 0x%08x, ret: %d\n", CM_CTRL_IOCTL_CMD, ret);
		return -1;
	}

	arg = 0;
	ret = ioctl(dev->fd, CM_CTRL_IOCTL_OFF, &arg);
	if (ret < 0) {
		loge("cm_control cmd: 0x%08x, ret: %d\n", CM_CTRL_IOCTL_OFF, ret);
		return -1;
	}

	return ret;
}