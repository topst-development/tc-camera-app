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
#include <video/tcc/cam_ipc.h>
#include <limits.h>
#include "vioc_global.h"
#include "basic_operation.h"
#include "log.h"
#include "cam_ipc.h"

#define DEVICE_NAME			("/dev/cam_ipc_ap")

int32_t cam_ipc_set_wmixer_ovp(struct cam_ipc *dev, int32_t ovp)
{
	struct cam_ipc_data		data		= { 0, };
	char				name[1024]	= "";
	int32_t				ret		= 0;

	(void)memset((void *)&data, 0, sizeof(data));

	/* coverity[misra_c_2012_rule_21_6_violation : FALSE] */
	ret = sprintf(name, "%s", DEVICE_NAME);
	if (ret < 0) {
		loge("sprintf, ret: %d\n", ret);
		ret = -1;
	} else {
		logd("device name: %s\n", name);
		dev->fd = open(name, O_RDWR);
		if (dev->fd < 0) {
			loge("fd(%d) is wrong", dev->fd);
			ret = -1;
		} else {
			(void)memset(&data, 0x0, sizeof(struct cam_ipc_data));
			data.cmd = ((uint32_t)CAM_IPC_CMD_OVP << 16U) & 0xFFFF0000U;
			//data.wmix_ch = VIOC_WMIX0;
			data.wmix_ch = 0x0400;
			data.ovp = s32_to_u32(ovp);
			data.data_len = 2;

			/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
			/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
			/* coverity[misra_c_2012_rule_12_2_violation : FALSE] */
			ret = ioctl(dev->fd, IOCTL_CAM_IPC_SET_OVP, &data);
			if (ret <= 0) {
				loge("set ovp, ret: %d\n", ret);
				ret = -1;
			}

			(void)close(dev->fd);
		}
	}

	return ret;
}
