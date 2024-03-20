/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) Telechips Inc.
 */

#ifndef CAM_IPC_H
#define CAM_IPC_H

#include <stdint.h>

struct cam_ipc {
	// vioc manager device
	int32_t		fd;
};

struct cam_ipc_data {
	uint32_t	cmd;
	uint32_t	wmix_ch;
	uint32_t	ovp;
	uint32_t	data_len;
};

extern int32_t	cam_ipc_set_wmixer_ovp(struct cam_ipc *dev, int32_t ovp);

#endif//CAM_IPC_H
