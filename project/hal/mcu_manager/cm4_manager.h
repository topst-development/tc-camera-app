/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) Telechips Inc.
 */

#ifndef CM4_MANAGER_H
#define CM4_MANAGER_H

#include <stdint.h>

#define CM_CTRL_IOCTL_OFF		(0)
#define CM_CTRL_IOCTL_ON		(1)
#define CM_CTRL_IOCTL_CMD		(3)
#define CM_CTRL_IOCTL_KNOCK		(4)

enum {
	CMD_STOP_EARLY_CAMERA	= 0x51,
	CMD_EXIT_EARLY_CAMERA	= 0x52,
};

struct cm4_manager {
	// cm4 manager device
	int32_t		fd;
};

extern int cm4_manager_open_device(struct cm4_manager *dev);
extern int cm4_manager_close_device(struct cm4_manager *dev);
extern int cm4_manager_status_cm_control(struct cm4_manager *dev);
extern int cm4_manager_disable_cm_control(struct cm4_manager *dev, unsigned long arg);

#endif//CM4_MANAGER_H
