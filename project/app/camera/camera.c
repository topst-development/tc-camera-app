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
#include <sys/poll.h>
#include <sys/mman.h>

#include <asm-generic/errno-base.h>
#include <video/tcc/tcc_cam_ioctrl.h>

#include "log.h"
#include "klog.h"
#include "message_queue.h"
#include "v4l2.h"
#include "list.h"
#include "switch.h"
#include "cm4_manager.h"
#include "camera.h"
#include "lut.h"
#include "basic_operation.h"

#define MAX_HANDOVER_STEP	5
#define DEVICES_TO_OPEN		5

enum CAMERA_CMD {
	CAMERA_CMD_INIT,
	CAMERA_CMD_HANDOVER,
	CAMERA_CMD_START_PREVIEW,
	CAMERA_CMD_PAUSE_PREVIEW,
	CAMERA_CMD_RESUME_PREVIEW,
	CAMERA_CMD_STOP_PREVIEW,
	CAMERA_CMD_START_RECORD,
	CAMERA_CMD_STOP_RECORD,
	CAMERA_CMD_SNAPSHOT_RECORD,
	CAMERA_CMD_KILL,
};

void camera_init_parameters(struct camera *dev)
{
	const unsigned char		*src		= NULL;
	unsigned char			*dst		= NULL;

	(void)memset(dev, 0, sizeof(struct camera));

	/* lookup table */
	src = lut_c0;
	dst = dev->vin.lut.table[LUT_COLOR_0];
	(void)memcpy(dst, src, sizeof(lut_c0));

	src = lut_c1;
	dst = dev->vin.lut.table[LUT_COLOR_1];
	(void)memcpy(dst, src, sizeof(lut_c1));

	src = lut_c2;
	dst = dev->vin.lut.table[LUT_COLOR_2];
	(void)memcpy(dst, src, sizeof(lut_c2));

	/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
	/* coverity[misra_c_2012_rule_10_8_violation : FALSE] */
	dev->vin.comp_flags		= (unsigned int)V4L2_SEL_FLAG_GE;
	dev->preview_width		= DISPLAY_SCREEN_WIDTH;
	dev->preview_height		= DISPLAY_SCREEN_HEIGHT;
	dev->preview_format		= V4L2_PIX_FMT_RGB32;
	dev->vin.io_mode		= (uint32_t)V4L2_MEMORY_MMAP;
	dev->vin.framerate		= 60;
	dev->vout.ovl.wmix_fovp	= -1;	/* the initial ovp must be -1 */
	dev->vout.ovl.wmix_bovp	= -1;	/* the initial ovp must be -1 */
	dev->recovery			= 1;
}

void camera_show_parameters(const struct camera *dev)
{
	logi("%20s: %d\n", "Switch Device", dev->sw.id);

	logi("%20s: %d\n", "Video-Input Path", dev->vin.capture.id);

	/* crop & compose */
	logi("%20s: (%d * %d) %u * %u\n", "Crop",
		dev->vin.crop.left, dev->vin.crop.top, dev->vin.crop.width, dev->vin.crop.height);
	logi("%20s: (%d * %d) %u * %u\n", "Compose",
		dev->vin.comp.left, dev->vin.comp.top, dev->vin.comp.width, dev->vin.comp.height);

	switch (dev->vin.io_mode) {
	case (unsigned int)V4L2_MEMORY_MMAP:
		logi("%20s: %s\n", "IO Mode", "V4L2_MEMORY_MMAP");
		break;
	case (unsigned int)V4L2_MEMORY_USERPTR:
		logi("%20s: %s\n", "IO Mode", "V4L2_MEMORY_USERPTR");
		break;
	case (unsigned int)V4L2_MEMORY_DMABUF:
		logi("%20s: %s\n", "IO Mode", "V4L2_MEMORY_DMABUF");
		break;
	default:
		loge("%20s: %s\n", "IO Mode", "WRONG");
		break;
	}

	logi("%20s: (%d * %d) %d * %d\n", "Preview Area",
		dev->preview_posx, dev->preview_posy,
		dev->preview_width, dev->preview_height);

	logi("%20s: %s\n", "Preview Format",
		v4l2_get_format_name_by_v4l2_format(dev->preview_format));

#if defined(USE_G2D)
	switch (dev->preview_rot) {
	case (unsigned int)NOOP:
		logi("%20s: %s\n", "Preview Rotation", "NOOP");
		break;
	case (unsigned int)ROTATE_90:
		logi("%20s: %s\n", "Preview Rotation", "ROTATE_90");
		break;
	case (unsigned int)ROTATE_180:
		logi("%20s: %s\n", "Preview Rotation", "ROTATE_180");
		break;
	case (unsigned int)ROTATE_270:
		logi("%20s: %s\n", "Preview Rotation", "ROTATE_270");
		break;
	default:
		loge("preview_rot(%u) is wrong\n", dev->preview_rot);
		break;
	}
#endif//defined(USE_G2D)

	logi("%20s: %d\n", "Video-Output Device", dev->vout.ovl.id);

	logi("%20s: %s\n", "Application Mode",
		((dev->application_mode == (int)APPLICATION_MODE_NORMAL)
		? "APPLICATION_MODE_NORMAL"
		: "APPLICATION_MODE_SIMPLE_ON_OFF"));

	if ((dev->vout.ovl.wmix_fovp != -1) && (dev->vout.ovl.wmix_bovp != -1)) {
		logi("%20s: %d\n", "Foreground Ovp", dev->vout.ovl.wmix_fovp);
		logi("%20s: %d\n", "Background Ovp", dev->vout.ovl.wmix_bovp);
	}
	logi("%20s: %u\n", "Ignore Ovp", dev->vout.ignore_ovp);

	logi("%20s: %d\n", "Recovery", dev->recovery);
}

static int camera_open_switch(struct camera *dev)
{
	struct switch_t			*sw		= NULL;
	int				ret		= 0;

	sw		= &dev->sw;

	if (sw->id < 0) {
		logd("switch id is %d, run-by-command mode\n", sw->id);
		ret = -1;
	} else {
		ret = switch_open_device(sw);
		if (ret < 0) {
			loge("switch_open_device, ret: %d\n", ret);
			ret = -1;
		}
	}

	return ret;
}

static int camera_open_vin(struct camera *dev)
{
	struct video_input		*vin		= NULL;
	int				ret		= 0;

	vin		= &dev->vin;

	ret = video_input_open_device(vin);
	if (ret < 0) {
		loge("video_input_open_device, ret: %d\n", ret);
		ret = -1;
	} else {
		ret = video_input_query_capabilities(vin);
		if (ret < 0) {
			loge("video_input_query_capabilities, ret: %d\n", ret);
			ret = -1;
		}
	}
	return ret;
}

static int camera_open_vout(struct camera *dev)
{
	struct video_output		*vout		= NULL;
	int				ret		= 0;

	vout		= &dev->vout;

	ret = video_output_open_device(vout);
	if (ret < 0) {
		loge("video_output_open_device, ret: %d\n", ret);
		ret = -1;
	} else {
		ret = video_output_init_params(vout);
		if (ret < 0) {
			loge("video_output_init_params, ret: %d\n", ret);
			ret = -1;
		}
	}

	return ret;
}

/* coverity[misra_c_2012_rule_8_13_violation : FALSE] */
static int camera_open_g2d(struct camera *dev)
{
#if defined(USE_G2D)
	struct graphic2d		*g2d		= NULL;
#endif//defined(USE_G2D)
	int				ret		= 0;

#if defined(USE_G2D)
	g2d		= &dev->g2d;

	ret = g2d_open(g2d);
	if (ret < 0) {
		logw("g2d_open, ret: %d\n", ret);
		ret = -1;
	}
#else
	(void)dev;
#endif//defined(USE_G2D)

	return ret;
}

static int camera_open_msgq(struct camera *dev)
{
	int				ret		= 0;

	ret = message_queue_init(&dev->msger);
	if (ret < 0) {
		loge("message_queue_init, ret: %d\n", ret);
		ret = -1;
	}

	return ret;
}

static int camera_open_cm4_mgr(const struct camera *dev)
{
#if defined(CM4_MANAGER_SUPPORT)
	struct cm4_manager	*cm4mgr		= NULL;
#endif//defined(CM4_MANAGER_SUPPORT)
	int				ret		= 0;

#if defined(CM4_MANAGER_SUPPORT)
	cm4mgr		= &dev->cm4mgr;

	ret = cm4_manager_open_device(cm4mgr);
	if (ret < 0) {
		logw("cm4_manager_open, ret: %d\n", ret);
		ret = -1;
	}
#else
	(void)dev;
#endif//defined(CM4_MANAGER_SUPPORT)

	return ret;
}

/*
 * The way of fixing coverity warnings is separating a function
 * 'camera_open_devices' into several steps. But, this should be changed with
 * device abstraction with discussing whole architecture of the application. So,
 * I will mark this as FIXME.
 */
static int (*open_devices[DEVICES_TO_OPEN])(struct camera *dev) = {
	camera_open_switch,
	camera_open_vin,
	camera_open_vout,
	camera_open_g2d,
	camera_open_msgq,
	camera_open_cm4_mgr,
};

int camera_open_devices(struct camera *dev)
{
	int				idx		= 0;
	int				ret		= 0;

	for (idx = 0; idx < DEVICES_TO_OPEN; idx++) {
		ret = open_devices[idx](dev);
		if (ret < 0) {
			loge("Failed to open device at %d\n", (idx + 1));
			break;
		}
	}

	return ret;
}

void camera_close_devices(const struct camera *dev)
{
	const struct switch_t		*sw		= NULL;
	const struct video_input	*vin		= NULL;
#if defined(USE_G2D)
	const struct graphic2d		*g2d		= NULL;
#endif//defined(USE_G2D)
	const struct video_output	*vout		= NULL;
#if defined(CM4_MANAGER_SUPPORT)
	const struct cm4_manager	*cm4mgr		= NULL;
#endif//defined(CM4_MANAGER_SUPPORT)
	int				ret		= 0;

	sw		= &dev->sw;
	vin		= &dev->vin;
#if defined(USE_G2D)
	g2d		= &dev->g2d;
#endif//defined(USE_G2D)
	vout		= &dev->vout;
#if defined(CM4_MANAGER_SUPPORT)
	cm4mgr		= &dev->cm4mgr;
#endif//defined(CM4_MANAGER_SUPPORT)

#if defined(CM4_MANAGER_SUPPORT)
	ret = cm4_manager_close_device(&dev->cm4mgr);
	if (ret < 0) {
		/* error */
		loge("cm4_manager_close_device, ret: %d\n", ret);
	}
#endif//defined(CM4_MANAGER_SUPPORT)

	ret = message_queue_deinit(&dev->msger);
	if (ret < 0) {
		/* error */
		loge("message_queue_deinit, ret: %d\n", ret);
	}

#if defined(USE_G2D)
	/* close a g2d device */
	ret = g2d_close(g2d);
	if (ret < 0) {
		/* error */
		logw("g2d_close, ret: %d\n", ret);
	}
#endif//defined(USE_G2D)

	ret = video_output_close_device(vout);
	if (ret < 0) {
		/* error */
		loge("video_output_close_device, ret: %d\n", ret);
	}

	ret = video_input_close_device(vin);
	if (ret < 0) {
		/* error */
		loge("video_input_close_device, ret: %d\n", ret);
	}

	if (sw->id < 0) {
		/* sw switch */
		logd("switch id is %d, there is no device to close.\n", sw->id);
	} else {
		ret = switch_close_device(sw);
		if (ret < 0) {
			/* error */
			loge("switch_close_device, ret: %d\n", ret);
		}
	}
}

/* coverity[misra_c_2012_rule_8_13_violation : FALSE] */
static int camera_show_lastframe(struct camera *dev)
{
	const struct video_input	*vin		= NULL;
	const struct video_output	*vout		= NULL;
	struct video_output_buffer	vout_buf	= { 0, };
	unsigned int			lastframe	= 0;
	int				ret		= 0;

	vin		= &dev->vin;
	vout		= &dev->vout;

	ret = video_input_create_lastframe(vin);
	if (ret < 0) {
		loge("video_input_create_lastframe, ret: %d\n", ret);
	} else {
		ret = video_input_get_lastframe_addrs(vin, &lastframe);
		if (ret < 0) {
			loge("video_input_get_lastframe_addrs, ret: %d\n", ret);
		} else {
			logd("address of lastframe: 0x%08x\n", lastframe);

			(void)memset((void *)&vout_buf, 0, sizeof(vout_buf));

			if ((vout_buf.width  <= (unsigned int)DISPLAY_SCREEN_WIDTH) &&
			    (vout_buf.height <= (unsigned int)DISPLAY_SCREEN_HEIGHT)) {
				vout_buf.width		= dev->preview_width;
				vout_buf.height		= dev->preview_height;
				/* coverity[misra_c_2012_rule_10_8_violation : FALSE] */
				vout_buf.posx		= (dev->preview_posx == -1)
					? (int)(((unsigned int)DISPLAY_SCREEN_WIDTH  - vout_buf.width) / 2U)
					: dev->preview_posx;
				/* coverity[misra_c_2012_rule_10_8_violation : FALSE] */
				vout_buf.posy		= (dev->preview_posy == -1)
					? (int)(((unsigned int)DISPLAY_SCREEN_HEIGHT - vout_buf.height) / 2U)
					: dev->preview_posy;
				vout_buf.format		= dev->preview_format;
				vout_buf.addrs[0]	= lastframe;
				vout_buf.addrs[1]	= 0U;
				vout_buf.addrs[2]	= 0U;
			} else {
				/* error */
				loge("Wrong size is configured in vout_buf\n");
			}
		}
	}

	return video_output_preview_buffer(vout, &vout_buf);
}

/**
 * @brief Getting OVP values from application arguments
 *
 * @param dev camera device instance
 * @param is_shown a flag of whether to show preview
 * @return int WMIXER OVP (-1 on error)
 */
static inline int camera_get_corresponding_ovp(const struct camera *dev, unsigned int is_shown)
{
	int				rvalue		= 0;

	switch (is_shown) {
	case (unsigned int)VOUT_WIND_FOREGROUND:
		rvalue = dev->vout.ovl.wmix_fovp;
		break;
	case (unsigned int)VOUT_WIND_BACKGROUND:
		rvalue = dev->vout.ovl.wmix_bovp;
		break;
	default:
		rvalue = -1;
		break;
	}

	return rvalue;
}

static int camera_set_ovp_with_cam_ipc(struct camera *dev, unsigned int show)
{
	int				ret		= 0;
#if defined(CAM_IPC_SUPPORT)
	struct cam_ipc			*camipc		= NULL;
	int				ovp		= 0;

	camipc		= &dev->camipc;

	ovp = camera_get_corresponding_ovp(dev, show);
	ret = cam_ipc_set_wmixer_ovp(camipc, ovp);
	if (ret < 0) {
		loge("TIMEOUT - cam_ipc_set_wmixer_ovp\n");
		ret = -1;
	}
#endif//defined(CAM_IPC_SUPPORT)
	return ret;
}

static int camera_configure_vout_ovp(struct camera *dev, unsigned int show)
{
	const struct video_output	*vout		= NULL;
	int				priority	= 0;
	int				fallback	= 1;
	int				ret		= 0;

	vout		= &dev->vout;

	priority = (show == 1U) ? (int)VOUT_WIND_FOREGROUND : (int)VOUT_WIND_BACKGROUND;

#if defined(CAM_IPC_SUPPORT)
	fallback = camera_set_ovp_with_cam_ipc(dev, show);
#endif//defined(CAM_IPC_SUPPORT)
	if (fallback != 0) {
		ret = video_output_set_window_priority(vout, priority);
		if (ret < 0) {
			loge("video_output_set_window_priority, ret: %d\n", ret);
			ret = -1;
		}
	}

	if (ret == 0) {
		ret = video_output_get_window_priority(vout, &priority);
		if (ret < 0) {
			loge("video_output_get_window_priority, ret: %d\n", ret);
			ret = -1;
		}
	}

	return ret;
}

static int camera_configure_vout_path(const struct camera *dev, unsigned int show)
{
	const struct video_output	*vout		= NULL;
	int				ret		= 0;

	vout		= &dev->vout;

	if (show != (unsigned int)MODE_PREVIEW_STARTED) {
		ret = video_output_disable_path(vout);
		if (ret < 0) {
			loge("video_output_disable_path, ret: %d\n", ret);
			ret = -1;
		}
	}

	return ret;
}

static void camera_show_video_output(struct camera *dev, unsigned int show)
{
	const struct video_output	*vout		= NULL;
	int				ret		= 0;

	vout		= &dev->vout;

	logk("> before camera_show_video_output");

	if (vout->ignore_ovp == 1U) {
		logi("The command to control ovp is ignored\n");
	} else {
		ret = camera_configure_vout_ovp(dev, show);
		if (ret < 0) {
			/* error */
			loge("camera_configure_vout_ovp, ret: %d\n", ret);
		}

		ret = camera_configure_vout_path(dev, show);
		if (ret < 0) {
			/* error */
			loge("camera_configure_vout_path, ret: %d\n", ret);
		}
	}

	logk("> after camera_show_video_output");
}

static void handover_cm4_env(const struct camera *dev)
{
#if defined(CM4_MANAGER_SUPPORT)
	int				cm4_status	= -1;
	int				vin_path_status	= 0;
	unsigned long 			arg 		= 0;

	cm4_status	= cm4_manager_status_cm_control(&dev->cm4mgr);
	vin_path_status = video_input_check_path_status(&dev->vin);

	if (cm4_status == 0) {
		if (vin_path_status == 1) {
			/* exit */
			arg = CMD_EXIT_EARLY_CAMERA;
		} else {
			/* stop */
			arg = CMD_STOP_EARLY_CAMERA;
		}

		cm4_manager_disable_cm_control(&dev->cm4mgr, arg);
	}
#else
	(void)dev;
#endif
}

/* coverity[misra_c_2012_rule_8_13_violation : FALSE] */
static int set_lastframe(struct camera *dev)
{
	int				ret		= 0;

	ret = camera_show_lastframe(dev);
	if (ret < 0) {
		loge("camera_show_lastframe, ret: %d\n", ret);
		ret = -1;
	}

	return ret;
}

static int set_handover_flag(struct camera *dev)
{
	const struct video_input	*vin		= NULL;
	int				ret		= 0;

	vin		= &dev->vin;

	dev->is_handover_need = (int)V4L2_CAP_CTRL_SKIP_ALL;
	ret = video_input_set_handover(vin, dev->is_handover_need);
	if (ret < 0) {
		loge("video_input_set_handover, ret: %d\n", ret);
		ret = -1;
	}

	return ret;
}

/* coverity[misra_c_2012_rule_8_13_violation : FALSE] */
static int set_lut(struct camera *dev)
{
	const struct video_input	*vin		= NULL;
	int				ret		= 0;

	vin		= &dev->vin;

	ret = video_input_set_lut(vin, &vin->lut);
	if (ret < 0) {
		loge("video_input_set_lut, ret: %d\n", ret);
		ret = -1;
	}

	return ret;
}

static int start_stream(struct camera *dev)
{
	struct video_input		*vin		= NULL;
	int				ret		= 0;

	vin		= &dev->vin;

	vin->frame_width	= dev->preview_width;
	vin->frame_height	= dev->preview_height;
	vin->format		= dev->preview_format;

	/* wait at least 3 frames is shown; 0.100 s (based on 60 fps) */
	(void)usleep(100 * 1000);

	ret = video_input_start_preview(vin);
	if (ret < 0) {
		loge("video_input_start_preview, ret: %d\n", ret);
		ret = -1;
	}

	return ret;
}

static int unset_handover_flag(struct camera *dev)
{
	const struct video_input	*vin		= NULL;
	int				ret		= 0;

	vin		= &dev->vin;

	dev->is_handover_need = (int)V4L2_CAP_CTRL_SKIP_NONE;
	ret = video_input_set_handover(vin, dev->is_handover_need);
	if (ret < 0) {
		loge("video_input_set_handover, ret: %d\n", ret);
		ret = -1;
	}

	logk("> handover is completed");

	return ret;
}

static void show_vout(struct camera *dev)
{
	dev->status = MODE_PREVIEW_STARTED;

	camera_show_video_output(dev, 1U);
}


static int (*handover_steps[])(struct camera *dev) = {
	set_lastframe,
	set_handover_flag,
	set_lut,
	start_stream,
	unset_handover_flag
};

static void handover_if_early_running(struct camera *dev)
{
	int				idx		= 0;
	int				ret		= 0;

	for (idx = 0; idx < MAX_HANDOVER_STEP; idx++) {
		ret = handover_steps[idx](dev);
		if (ret < 0) {
			loge("Failed to handover: at %d step\n", (idx + 1));
			break;
		}
	}

	if (ret == 0) {
		/* show */
		show_vout(dev);
	}
}

static void do_handover(struct camera *dev)
{
	const struct switch_t		*sw		= NULL;
	const struct video_input	*vin		= NULL;
	int				vin_path_status	= 0;
	int				reverse_status	= -1;

	sw		= &dev->sw;
	vin		= &dev->vin;

	/* camera system is not initialized */
	dev->initialized = 0;

	reverse_status = switch_get_state(sw);
	vin_path_status = video_input_check_path_status(vin);

	logi("VIN[%d] The video-input path was %s working.\n", vin->capture.id,
		(vin_path_status == 1) ? "already" : "NOT");

	/* handover from earlycamera by cortex-m on tcc897x_linux_ivi_k414 */
	if (dev->use_cm4 == 1) {
		/* handover */
		handover_cm4_env(dev);
	}

	if (vin_path_status == 1) {
		/* handover */
		handover_if_early_running(dev);
	}
}

static int prepare_g2d(const struct camera *dev)
{
#if defined(USE_G2D)
	struct graphic2d		*g2d		= NULL;
#endif//defined(USE_G2D)
	int				ret		= 0;

#if defined(USE_G2D)
	g2d		= &dev->g2d;

	ret = g2d_is_available(g2d);
	if (ret == 0) {
		ret = g2d_memory_allocate(g2d,
			(unsigned int)dev->preview_width, (unsigned int)dev->preview_height,
			v4l2_get_color_depth_by_v4l2_format(dev->preview_format));
		if (ret < 0) {
			loge("g2d_memory_allocate, ret: %d\n", ret);
			return ret;
		}
	}
#else
	(void)dev;
#endif//defined(USE_G2D)

	return ret;
}

static int do_start_preview(struct camera *dev)
{
	const struct video_input	*vin		= NULL;
	int				ret		= 0;

	vin		= &dev->vin;

	if (dev->status == MODE_PREVIEW_STARTED) {
		logd("The video-input path is working already\n");
		ret = -1;
	} else {
		/* camera system is not initialized */
		dev->initialized = 0;

		if ((prepare_g2d(dev) == 0) &&
			(set_lut(dev) == 0) &&
			(start_stream(dev) == 0)) {
			show_vout(dev);
		} else {
			/* error */
			loge("Failed to start preview\n");
		}
	}

	return ret;
}

static int do_stop_preview(struct camera *dev)
{
	struct video_input		*vin		= NULL;
#if defined(USE_G2D)
	struct graphic2d		*g2d		= NULL;
#endif//defined(USE_G2D)
	int				ret		= 0;

	vin		= &dev->vin;
#if defined(USE_G2D)
	g2d		= &dev->g2d;
#endif//defined(USE_G2D)

	if (dev->status != MODE_PREVIEW_STARTED) {
		logd("The video-input path is not working already\n");
		ret = -1;
	} else {
		/* hide video_output */
		camera_show_video_output(dev, 0U);

		ret = video_input_stop_preview(vin);
		if (ret < 0) {
			loge("video_input_stop_preview, ret: %d\n", ret);
			ret = -1;
		}

#if defined(USE_G2D)
		ret = g2d_is_available(g2d);
		if (ret == 0) {
			ret = g2d_memory_deallocate(g2d);
			if (ret < 0) {
				loge("g2d_memory_deallocate, ret: %d\n", ret);
				ret = -1;
			}
		}
#endif//defined(USE_G2D)
	}

	dev->status = MODE_PREVIEW_STOPPED;

	return ret;
}

static int camera_check_buffer(const struct camera *dev,
	const struct v4l2_buffer *buf)
{
	const struct video_input	*vin		= NULL;
	unsigned int			width		= 0;
	unsigned int			height		= 0;
	const unsigned char		*base_addr	= NULL;
	const void			*ptr1		= NULL;
	const void			*ptr2		= NULL;
	int				ret_cmp		= 0;
	int				ret		= 0;

	vin		= &dev->vin;
	width		= dev->preview_width;
	height		= dev->preview_height;

	logd("index: %u, paddr: %p, vaddr: %p\n",
		buf->index,
		vin->buffers[buf->index].paddrs[0].addr,
		vin->buffers[buf->index].vaddrs[0].addr);

	if ((width  < (unsigned int)MAX_LIMIT_WIDTH) &&
	    (height < (unsigned int)MAX_LIMIT_HEIGHT)) {
		/* coverity[misra_c_2012_rule_11_5_violation : FALSE] */
		base_addr	= (const unsigned char *)(vin->buffers[buf->index].vaddrs[0].addr);
		/* coverity[misra_c_2012_rule_18_4_violation : FALSE] */
		ptr1		= (const void *)(base_addr + ((height / 4U * 1U) * width));
		ptr2		= (const void *)(base_addr + ((height / 4U * 3U) * width));
		logd("base_addr: %p, ptr1: %p, ptr2: %p\n", base_addr, ptr1, ptr2);
	} else {
		width  = (unsigned int)DISPLAY_SCREEN_WIDTH;
		/* coverity[misra_c_2012_rule_2_2_violation : FALSE] */
		height = (unsigned int)DISPLAY_SCREEN_HEIGHT;
		loge("Using default size: %u * %u\n", width, height);
	}

	if (ptr1 != NULL) {
		ret_cmp = memcmp(ptr1, ptr2, width);
		if (ret_cmp == 0) {
			logd("memcmp, ret: %d\n", ret_cmp);
			ret = -1;
		}
	}

	return ret;
}

#if defined(USE_G2D)
int camera_rotate_buffer(struct camera *dev,
	const struct v4l2_buffer *buf)
{
	struct video_input		*vin		= NULL;
	struct graphic2d		*g2d		= NULL;
	struct video_output		*vout		= NULL;
	unsigned int			src_addr0	= 0;
	unsigned int			src_addr1	= 0;
	unsigned int			src_addr2	= 0;
	unsigned int			dst_addr0	= 0;
	unsigned int			dst_addr1	= 0;
	unsigned int			dst_addr2	= 0;
	unsigned int			img_width	= 0;
	unsigned int			img_height	= 0;
	unsigned int			crop_offx	= 0;
	unsigned int			crop_offy	= 0;
	unsigned int			crop_imgx	= 0;
	unsigned int			crop_imgy	= 0;
	unsigned int			angle		= 0;
	unsigned int			preview_width	= 0;
	unsigned int			preview_height	= 0;

	struct video_output_buffer	vout_buf	= { 0, };
	int				ret		= 0;

	vin		= &dev->vin;
	g2d		= &dev->g2d;
	vout		= &dev->vout;

	if (dev->preview_rot != 0U) {
		src_addr0	= (unsigned int)vin->buffers[buf->index].paddrs[0].addr;
		dst_addr0	= g2d_get_memory_address(g2d);

		img_width	= (unsigned int)dev->preview_width;
		img_height	= (unsigned int)dev->preview_height;
		crop_imgx	= img_width;
		crop_imgy	= img_height;
		angle		= dev->preview_rot;

		ret = g2d_rotation(g2d, src_addr0, 0, 0, dst_addr0, 0, 0,
			img_width, img_height, crop_offx, crop_offy,
			crop_imgx, crop_imgy, angle);

		if ((angle == (unsigned int)ROTATE_90) || (angle == (unsigned int)ROTATE_270)) {
			preview_width	= crop_imgy;
			preview_height	= crop_imgx;
		} else {
			preview_width	= crop_imgx;
			preview_height	= crop_imgy;
		}
	}
	logd("addr: 0x%08x, 0x%08x, 0x%08x\n",
		dst_addr0, dst_addr1, dst_addr2);

	(void)memset((void *)&vout_buf, 0, sizeof(vout_buf));
	vout_buf.width		= preview_width;
	vout_buf.height		= preview_height;
	vout_buf.posx		= (dev->preview_posx == -1)
		? (((unsigned int)DISPLAY_SCREEN_WIDTH  - preview_width)  / 2U)
		: (unsigned int)dev->preview_posx;
	vout_buf.posy		= (dev->preview_posy == -1)
		? (((unsigned int)DISPLAY_SCREEN_HEIGHT - preview_height) / 2U)
		: (unsigned int)dev->preview_posy;
	vout_buf.format		= dev->preview_format;
	vout_buf.addrs[0]	= dst_addr0;
	vout_buf.addrs[1]	= dst_addr1;
	vout_buf.addrs[2]	= dst_addr2;

	return ret;
}
#endif//defined(USE_G2D)

static int write_data(int fd, const char *buf, int buf_len)
{
	int size = 0;
	int ret = 0;
	int len;

	for (;;) {
		len = s64_to_s32(write(fd, &buf[size], s32_to_u64(buf_len - size)));
		if (len > 0) {
			size += len;
			if (size == buf_len) {
				ret = size;
			} else {
				continue;
			}
		} else if (len == 0) {
			ret = size;
		} else {
			ret = -1;
		}

		break;
	}

	return ret;
}

static void camera_save_buffer(struct camera *dev, const struct v4l2_buffer *buf)
{
	const struct video_input	*vin		= NULL;
	char				fname[32]	= "";
	int				fd		= 0;
	static unsigned int		capture_idx;
	int				ret		= 0;

	vin		= &dev->vin;

	if (dev->cnt_to_capture > 0) {
		/* coverity[misra_c_2012_rule_21_6_violation : FALSE] */
		ret = snprintf(fname, 32, "video_capture.%u", capture_idx);
		if (ret < 0) {
			loge("snprintf, ret: %d\n", ret);
			ret = -1;
		}

		/* coverity[misra_c_2012_rule_7_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
		fd = open(fname, O_RDWR | O_CREAT, 0755);
		if (fd < 0) {
			loge("Failed to open the file to capture\n");
		} else {
			unsigned int pixel_size = dev->preview_width * dev->preview_height * 4U;

			/* coverity[misra_c_2012_rule_11_5_violation : FALSE] */
			/* coverity[cert_exp37_c_violation : FALSE] */
			if (pixel_size < (unsigned int)INT_MAX) {
				/* write */
				/* coverity[misra_c_2012_rule_11_5_violation : FALSE] */
				/* coverity[cert_exp37_c_violation : FALSE] */
				ret = write_data(fd, vin->buffers[buf->index].vaddrs[0].addr, (int)pixel_size);
			} else {
				/* error */
				ret = -1;
			}

			if (ret == 0) {
				/* error */
				loge("Failed to write file to %s\n", fname);
			} else {
				/* pass */
				logd("data (%d) is written\n", ret);
			}
			(void)close(fd);
		}

		dev->cnt_to_capture = dev->cnt_to_capture - 1;
		/* coverity[misra_c_2012_rule_10_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_1_violation : FALSE] */
		capture_idx = add_u32(capture_idx, 1);
	}
}

static int camera_show_buffer(const struct camera *dev,
	const struct v4l2_buffer *buf)
{
	const struct video_input	*vin		= NULL;
	const struct video_output	*vout		= NULL;
	struct video_output_buffer	vout_buf	= { 0, };
	int				ret		= 0;

	vin		= &dev->vin;
	vout		= &dev->vout;

	(void)memset((void *)&vout_buf, 0, sizeof(vout_buf));

	vout_buf.width		= dev->preview_width;
	vout_buf.height		= dev->preview_height;
	if ((vout_buf.width  <= (unsigned int)DISPLAY_SCREEN_WIDTH) &&
	    (vout_buf.height <= (unsigned int)DISPLAY_SCREEN_HEIGHT)) {
		/* coverity[misra_c_2012_rule_10_8_violation : FALSE] */
		vout_buf.posx		= (dev->preview_posx == -1)
			? (int)(((unsigned int)DISPLAY_SCREEN_WIDTH  - vout_buf.width)  / 2U)
			: dev->preview_posx;
		/* coverity[misra_c_2012_rule_10_8_violation : FALSE] */
		vout_buf.posy		= (dev->preview_posy == -1)
			? (int)(((unsigned int)DISPLAY_SCREEN_HEIGHT - vout_buf.height) / 2U)
			: dev->preview_posy;
		vout_buf.format		= dev->preview_format;
		/* coverity[misra_c_2012_rule_11_6_violation : FALSE] */
		/* coverity[cert_int36_c_violation : FALSE] */
		/* coverity[pointer_conversion_loses_bits : FALSE] */
		vout_buf.addrs[0]	= (unsigned int)vin->buffers[buf->index].paddrs[0].addr;
		/* coverity[misra_c_2012_rule_11_6_violation : FALSE] */
		/* coverity[cert_int36_c_violation : FALSE] */
		/* coverity[pointer_conversion_loses_bits : FALSE] */
		vout_buf.addrs[1]	= (unsigned int)vin->buffers[buf->index].paddrs[1].addr;
		/* coverity[misra_c_2012_rule_11_6_violation : FALSE] */
		/* coverity[cert_int36_c_violation : FALSE] */
		/* coverity[pointer_conversion_loses_bits : FALSE] */
		vout_buf.addrs[2]	= (unsigned int)vin->buffers[buf->index].paddrs[2].addr;

		ret = video_output_preview_buffer(vout, &vout_buf);
		if (ret < 0) {
			loge("Failed on video_output_preview_buffer\n");
			ret = -1;
		}
	} else {
		/* error */
		loge("Wrong size is configured in vout_buf size\n");
	}

	return ret;
}

static void check_recovery(struct camera *dev, int vin_path_status)
{
	const struct video_input	*vin		= NULL;
	static int			idxRecovery;
	int				nMaxRecovery	= 3;
	int				vs_status	= 0;
	int				ret		= 0;

	vin		= &dev->vin;

	if (vin_path_status == 1) {
		/* reset count of recovery */
		idxRecovery = 0;
	} else {
		if (idxRecovery < INT_MAX) {
			/* increase count of recovery */
			idxRecovery++;
		} else {
			loge("Overflow occurs in idxRecovery\n");
			idxRecovery = 0;
		}

		/* check video source's status */
		vs_status = video_input_check_source_status(vin);

		/* judge if recovery is needed */
		logi("vs_status: %d, recovery_count: %d / %d\n",
			vs_status, idxRecovery, nMaxRecovery);
		if ((vs_status != 1) || (nMaxRecovery <= idxRecovery)) {
			if (dev->recovery == 1) {
				loge("The video-input path is NOT working.\n");

				loge("It will be recovered soon.\n");

				ret = do_stop_preview(dev);
				if (ret != 0) {
					/* error */
					loge("do_stop_preview, ret: %d\n", ret);
				}

				ret = do_start_preview(dev);
				if (ret != 0) {
					/* error */
					loge("do_start_preview, ret: %d\n", ret);
				}
			}

			idxRecovery = 0;
		}
	}
}

static void check_buffer_and_set_flag(struct camera *dev,
	const struct v4l2_buffer *buf)
{
	int				ret_chk		= 0;

	ret_chk = camera_check_buffer(dev, buf);
	if (ret_chk < 0) {
		/* result of rotation */
		logd("camera_check_buffer, ret: %d\n", ret_chk);
	} else {
		if (dev->initialized == 0U) {
			/* the path is initialized to display */
			dev->initialized = 1;
			logi("the path is initialized to display\n");
		}
	}

}

static void release_buffer(const struct camera *dev, struct v4l2_buffer *buf)
{
	const struct video_input	*vin		= NULL;
	int				ret		= 0;

	vin		= &dev->vin;

	ret = video_input_qbuf(vin, buf);
	if (ret < 0) {
		/* result of show */
		logw("video_input_qbuf, ret: %d\n", ret);
	}
}

static void handle_initialized(struct camera *dev, const struct v4l2_buffer *buf)
{
	int				ret		= 0;

	if (dev->initialized == 1U) {
#if defined(USE_G2D)
		ret = camera_rotate_buffer(dev, buf);
		if (ret < 0) {
			/* result of rotation */
			logw("camera_rotate_buffer, ret: %d\n", ret);
		}
#endif//defined(USE_G2D)

		camera_save_buffer(dev, buf);
		ret = camera_show_buffer(dev, buf);
		if (ret < 0) {
			/* result of show */
			logw("camera_show_buffer, ret: %d\n", ret);
		}
	}
}

static void release_preview_buffer(struct camera *dev,
	struct v4l2_buffer *buf, struct buffer_t *fentry)
{
	list_del(&fentry->list);

	buf = &fentry->v4l2_buf;
	check_buffer_and_set_flag(dev, buf);
	handle_initialized(dev, buf);

	release_buffer(dev, buf);
}

static void handle_preview_buffer(struct camera *dev, int *vin_path_status)
{
	struct video_input		*vin		= NULL;
	struct v4l2_buffer		buf		= { 0, };
	struct buffer_t			*fentry		= NULL;
	int				ret		= 0;

	vin		= &dev->vin;

	ret = video_input_dqbuf(vin, &buf);
	if (ret == 0) {
		*vin_path_status = 1;

		vin->buffers[buf.index].v4l2_buf = buf;
		INIT_LIST_HEAD(&vin->buffers[buf.index].list);

		/* push a new buffer to tail of buf_list to display */
		list_add_tail(&vin->buffers[buf.index].list, &vin->buf_list);

		/* coverity[misra_c_2012_rule_8_13_violation : FALSE] */
		/* coverity[misra_c_2012_rule_11_3_violation : FALSE] */
		/* coverity[misra_c_2012_rule_11_8_violation : FALSE] */
		/* coverity[misra_c_2012_rule_12_1_violation : FALSE] */
		/* coverity[misra_c_2012_rule_18_4_violation : FALSE] */
		/* coverity[misra_c_2012_rule_20_7_violation : FALSE] */
		/* coverity[cert_exp36_c_violation : FALSE] */
		/* coverity[cert_exp40_c_violation : FALSE] */
		fentry = list_first_entry_or_null(&vin->buf_list, struct buffer_t, list);

		logd("fentry - index: %d, phy_addr: %p, %p, %p\n",
			fentry->v4l2_buf.index,
			fentry->paddrs[0].addr,
			fentry->paddrs[1].addr,
			fentry->paddrs[2].addr);

		if (list_is_singular(&vin->buf_list)) {
			/* buf_list has only one child */
			logi("buf_list has only one child\n");
		} else {
			/* coverity[cert_exp39_c_violation : FALSE] */
			release_preview_buffer(dev, &buf, fentry);
		}
	}
}

static void camera_preview_buffer(struct camera *dev)
{
	const struct video_input	*vin		= NULL;
	int				pollin		= 0;
	int				vin_path_status	= 0;

	vin		= &dev->vin;

	pollin = video_input_poll(vin);
	if (pollin != 1) {
		/* error */
		logd("video_input_poll, ret: %d\n", pollin);
	} else {
		/* handle buffer */
		handle_preview_buffer(dev, &vin_path_status);
	}

	check_recovery(dev, vin_path_status);
}

static int handle_a_message(struct camera *dev, struct message *msg)
{
	int				ret		= 0;

	if (message_queue_is_empty(&dev->msger.cmd) != 1) {
		(void)message_queue_get(&dev->msger.cmd, msg);
		logd("cmd: 0x%08x\n", msg->command);

		switch (msg->command) {
		case (unsigned int)CAMERA_CMD_HANDOVER:
			logk("> before do_handover");
			do_handover(dev);
			logk("> after do_handover");
			break;
		case (unsigned int)CAMERA_CMD_START_PREVIEW:
			logk("> before do_start_preview");
			ret = do_start_preview(dev);
			if (ret != 0) {
				/* error */
				loge("do_start_preview, ret: %d\n", ret);
			}
			logk("> after do_start_preview");
			break;
		case (unsigned int)CAMERA_CMD_STOP_PREVIEW:
			logk("> before do_stop_preview");
			ret = do_stop_preview(dev);
			if (ret != 0) {
				/* error */
				loge("do_stop_preview, ret: %d\n", ret);
			}
			logk("> after do_stop_preview");
			break;
		default:
			logd("Nothing to handle ioctl cmd\n");
			break;
		}

		msg->command |= 0x80000000U;
		logd("ack: 0x%08x\n", msg->command);
		(void)message_queue_put(&dev->msger.ack, msg);
	}

	return ret;
}

static void *threadMessageHandle(void *param)
{
	struct camera			*dev		= NULL;
	const struct video_input	*vin		= NULL;
	struct message			msg		= { 0, };
	int				ret		= 0;

	/* coverity[misra_c_2012_rule_11_5_violation : FALSE] */
	dev	= (struct camera *)param;
	vin	= &dev->vin;

	while (dev->is_message_handle_thread_enabled == 1) {
		(void)memset((void *)&msg, 0, sizeof(msg));

		ret = handle_a_message(dev, &msg);

		if (dev->status == MODE_PREVIEW_STARTED) {
			/* buffer switch */
			camera_preview_buffer(dev);
		}

		(void)usleep(16 * 1000);
	}

	return (void *)NULL;
}

static int message_queue_put_and_get_msg(const struct camera *dev, unsigned int command)
{
	struct message			msg		= { 0, };
	int				ret		= 0;

	if (dev->is_message_handle_thread_enabled == 0) {
		loge("Message Handling Thread is NOT active.");
		ret = -1;
	} else {
		(void)memset((void *)&msg, 0, sizeof(msg));

		msg.command = command;

		ret = message_queue_put(&dev->msger.cmd, &msg);
		logd("tx command: 0x%08x, ret: %d\n", msg.command, ret);

		ret = message_queue_get(&dev->msger.ack, &msg);
		logd("rx command: 0x%08x, ret: %d\n", msg.command, ret);
	}

	return ret;
}

int camera_handover(const struct camera *dev)
{
	int				ret		= 0;

	ret = message_queue_put_and_get_msg(dev, (unsigned int)CAMERA_CMD_HANDOVER);
	if (ret != 0) {
		logd("message_queue_put_and_get_msg, ret: %d\n", ret);
		ret = -1;
	}

	return ret;
}

int camera_start_preview(const struct camera *dev)
{
	int				ret		= 0;

	ret = message_queue_put_and_get_msg(dev, (unsigned int)CAMERA_CMD_START_PREVIEW);
	if (ret != 0) {
		logd("message_queue_put_and_get_msg, ret: %d\n", ret);
		ret = -1;
	}

	return ret;
}

int camera_stop_preview(const struct camera *dev)
{
	int				ret		= 0;

	ret = message_queue_put_and_get_msg(dev, (unsigned int)CAMERA_CMD_STOP_PREVIEW);
	if (ret != 0) {
		logd("message_queue_put_and_get_msg, ret: %d\n", ret);
		ret = -1;
	}

	return ret;
}

int camera_create_camera_thread(struct camera *dev)
{
	int				ret		= 0;

	dev->is_message_handle_thread_enabled = 1;
	ret = pthread_create(&dev->message_handle_thread, NULL, &threadMessageHandle, (void *)dev);
	if (ret < 0) {
		loge("pthread_create, ret: %d\n", ret);
		ret = -1;
	}

	return ret;
}

int camera_destroy_camera_thread(struct camera *dev)
{
	int				ret		= 0;

	dev->is_message_handle_thread_enabled = 0;
	ret = pthread_join(dev->message_handle_thread, NULL);
	if (ret != 0) {
		loge("pthread_join, ret: %d\n", ret);
		ret = -1;
	}

	return ret;
}
