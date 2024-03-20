/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) Telechips Inc.
 */

#ifndef V4L2_CAPTURE_H
#define V4L2_CAPTURE_H

#include <stdint.h>
#include <linux/videodev2.h>
#include <video/tcc/tcc_cam_ioctrl.h>

#include "list.h"

struct v4l2_capture {
	int				id;
	int				fd;
};

extern int v4l2_capture_open_device(struct v4l2_capture *dev);
extern int v4l2_capture_close_device(const struct v4l2_capture *dev);
extern int v4l2_capture_query_capabilities(const struct v4l2_capture *dev,
	struct v4l2_capability *cap);
extern int v4l2_capture_set_format(const struct v4l2_capture *dev,
	struct v4l2_format *fmt);
extern int v4l2_capture_reqbufs(const struct v4l2_capture *dev,
	struct v4l2_requestbuffers *req);
extern int v4l2_capture_querybuf(const struct v4l2_capture *dev,
	struct v4l2_buffer *buf);
extern int v4l2_capture_qbuf(const struct v4l2_capture *dev,
	struct v4l2_buffer *buf);
extern int v4l2_capture_expbuf(const struct v4l2_capture *dev,
	struct v4l2_exportbuffer *buf);
extern int v4l2_capture_dqbuf(const struct v4l2_capture *dev,
	struct v4l2_buffer *buf);
extern int v4l2_capture_streamon(const struct v4l2_capture *dev,
	int *arg);
extern int v4l2_capture_streamoff(const struct v4l2_capture *dev,
	int *arg);
extern int v4l2_capture_enuminput(const struct v4l2_capture *dev,
	struct v4l2_input *input);
extern int v4l2_capture_g_input(const struct v4l2_capture *dev,
	int *index);
extern int v4l2_capture_s_selection(const struct v4l2_capture *dev,
	struct v4l2_selection *sel);
extern int v4l2_capture_g_parm(const struct v4l2_capture *dev,
	struct v4l2_streamparm *parm);
extern int v4l2_capture_s_parm(const struct v4l2_capture *dev,
	struct v4l2_streamparm *parm);
extern int v4l2_capture_enum_framesize(const struct v4l2_capture *dev,
	struct v4l2_frmsizeenum *frmsizeenum);
extern int v4l2_capture_enum_frameintervals(const struct v4l2_capture *dev,
	struct v4l2_frmivalenum *frmivalenum);
extern int v4l2_capture_check_v4l2_dev_status(const struct v4l2_capture *dev,
	int *status);
extern int v4l2_capture_g_lastframe_addrs(const struct v4l2_capture *dev,
	unsigned int *addrs);
extern int v4l2_capture_create_lastframe(const struct v4l2_capture *dev,
	unsigned int *addrs);
extern int v4l2_capture_s_handover(const struct v4l2_capture *dev,
	int *handover);
extern int v4l2_capture_s_lut(const struct v4l2_capture *dev,
	struct vin_lut *lut);
extern int v4l2_capture_poll(const struct v4l2_capture *dev,
	int event, int timeout);

#endif//V4L2_CAPTURE_H
