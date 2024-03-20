/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) Telechips Inc.
 */

#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>
#include <pthread.h>

#include "message_queue.h"
#include "switch.h"
#include "video_input.h"
#include "video_output.h"

//#define USE_G2D
#if defined(USE_G2D)
#include "g2d.h"
#endif//defined(USE_G2D)

#define CAM_IPC_SUPPORT
#if defined(CAM_IPC_SUPPORT)
#include "cam_ipc.h"
#endif//defined(CAM_IPC_SUPPORT)

//#define CM4_MANAGER_SUPPORT
#if defined(CM4_MANAGER_SUPPORT)
#include "cm4_manager.h"
#endif//defined(CM4_MANAGER_SUPPORT)

/*
 *	IMPORTANT
 *
 *	USER MUST SET THE SIZE OF DISPLAY DEVICE.
 *	THE DEFAULT IS 1920 * 720 WHICH IS BASED ON TCC803X & TCC805X EVB
 */
#define DISPLAY_SCREEN_WIDTH		(1920)
#define DISPLAY_SCREEN_HEIGHT		(720)
#define MAX_LIMIT_WIDTH			(16384)
#define MAX_LIMIT_HEIGHT		(16384)

enum operation_status {
	MODE_INITIALIZED		= 0,
	MODE_PREVIEW_STARTED,
	MODE_PREVIEW_STOPPED,
	MODE_CAPTURE
};

enum {
	APPLICATION_MODE_NORMAL		= 0,
	APPLICATION_MODE_SIMPLE_ON_OFF,
};

struct camera {
	struct switch_t			sw;

	int				preview_posx;
	int				preview_posy;
	unsigned int			preview_width;
	unsigned int			preview_height;
	unsigned int			preview_format;
	/* 0: v4l2, 1: direct display */
	unsigned int			preview_method;
	unsigned int			preview_rot;
	/* 0: normal preview, 1: unit test */
	int				application_mode;

	struct video_input		vin;
#if defined(USE_G2D)
	struct graphic2d		g2d;
#endif
#if defined(CAM_IPC_SUPPORT)
	struct cam_ipc			camipc;
#endif

	/* operation mode */
	enum operation_status		status;

	struct video_output		vout;

	int				recovery;

	int				use_cm4;
#if defined(CM4_MANAGER_SUPPORT)
	struct cm4_manager              cm4mgr;
#endif

	/* camera system is ready to preview */
	unsigned int			initialized;

	int				is_handover_need;

	/* the number of frames to capture */
	int				cnt_to_capture;

	struct messenger		msger;

	pthread_t			message_handle_thread;
	int				is_message_handle_thread_enabled;
};

extern void camera_init_parameters(struct camera *dev);
extern void camera_show_parameters(const struct camera *dev);
extern int camera_open_devices(struct camera *dev);
extern void camera_close_devices(const struct camera *dev);
extern int camera_handover(const struct camera *dev);
extern int camera_start_preview(const struct camera *dev);
extern int camera_stop_preview(const struct camera *dev);
extern int camera_create_camera_thread(struct camera *dev);
extern int camera_destroy_camera_thread(struct camera *dev);

#endif//CAMERA_H
