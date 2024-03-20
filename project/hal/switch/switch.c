// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Telechips Inc.
 */

#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <misc/switch.h>

#include "log.h"
#include "switch.h"

#define DEVICE_NAME			("/dev/switch_gpio_reverse")

int switch_open_device(struct switch_t *dev)
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
			loge("sprintf, ret: %d\n", ret);
			ret = -1;
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

int switch_close_device(const struct switch_t *dev)
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

int switch_get_state(const struct switch_t *dev)
{
	int				arg		= 0;
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		ret = ioctl(dev->fd, SWITCH_IOCTL_CMD_GET_STATE, &arg);
		if (ret < 0) {
			loge("ret: %d\n", ret);
			ret = -1;
		} else {
			logd("state: %d\n", arg);
			ret = arg;
		}
	}

	return ret;
}
