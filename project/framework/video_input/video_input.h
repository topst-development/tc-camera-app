/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) Telechips Inc.
 */

#ifndef VIDEO_INPUT_H
#define VIDEO_INPUT_H

#include <stdint.h>
#include <linux/videodev2.h>

#include "v4l2_capture.h"
#include "list.h"

#define	VIDEO_CAPTURE_CAP		V4L2_CAP_VIDEO_CAPTURE_MPLANE
#if defined(VIDEO_CAPTURE_CAP)
#define	VIDEO_CAPTURE_BUF_TYPE		V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
#endif//defined(VIDEO_CAPTURE_CAP)

#define NUM_VIDBUF			(4)
#ifdef	VIDEO_MAX_PLANES
#undef	VIDEO_MAX_PLANES
#endif
#define	VIDEO_MAX_PLANES		(3)

struct rect {
	int				left;
	/* coverity[cert_dcl37_c_violation : FALSE] */
	int				top;
	unsigned int			width;
	unsigned int			height;
};

struct buf_addr {
	void				*addr;
	unsigned int			length;
};

struct buffer_t {
	struct v4l2_buffer		v4l2_buf;
	struct buf_addr			paddrs[VIDEO_MAX_PLANES];
	struct buf_addr			vaddrs[VIDEO_MAX_PLANES];
	unsigned int			fd[VIDEO_MAX_PLANES];
	struct list_head		list;
};

enum lut_color {
	LUT_COLOR_0,
	LUT_COLOR_1,
	LUT_COLOR_2,
	LUT_COLOR_MAX,
};

enum start_preview_steps {
	START_PREVIEW_INIT_FRMRATES,
	START_PREVIEW_INIT_BUFFERS,
	START_PREVIEW_SEL_CROP,
	START_PREVIEW_SEL_COMPOSE,
	START_PREVIEW_ENABLE_STREAM,
	START_PREVIEW_STEPS_MAX,
};

struct lookup_table {
	unsigned int enable[LUT_COLOR_MAX];
	unsigned char table[LUT_COLOR_MAX][256];
};

struct video_input {
	struct v4l2_capture		capture;

	struct lookup_table		lut;

	/* crop and compose */
	struct rect			crop;
	unsigned int			comp_flags;
	struct rect			comp;

	/* stream */
	unsigned int			frame_width;
	unsigned int			frame_height;
	unsigned int			format;
	unsigned int			framerate;

	unsigned int			io_mode;
	unsigned int			n_allocated_buf;
	struct buffer_t			*buffers;
	struct list_head		buf_list;
};

extern int video_input_open_device(struct video_input *dev);
extern int video_input_close_device(const struct video_input *dev);
extern int video_input_query_capabilities(const struct video_input *dev);
extern int video_input_set_format(const struct video_input *dev,
	unsigned int width, unsigned int height, unsigned int format);
extern int video_input_request_buffers(const struct video_input *dev,
	unsigned int mem, unsigned int count);
extern int video_input_query_buffer(const struct video_input *dev,
	struct v4l2_buffer *vid_buf);
extern int video_input_expbuf(const struct video_input *dev,
	struct v4l2_exportbuffer *pbuf);
extern int video_input_qbuf(const struct video_input *dev,
	struct v4l2_buffer *pbuf);
extern int video_input_dqbuf(const struct video_input *dev,
	struct v4l2_buffer *pbuf);
extern int video_input_start_stream(const struct video_input *dev);
extern int video_input_stop_stream(const struct video_input *dev);
extern int video_input_enum_input(const struct video_input *dev,
	struct v4l2_input *input);
extern int video_input_get_input(const struct video_input *dev,
	int *index);
extern int video_input_set_selection(const struct video_input *dev,
	unsigned int target, unsigned int flags, const struct rect *r);
extern int video_input_get_framerate(const struct video_input *dev,
	unsigned int *framerate);
extern int video_input_set_framerate(const struct video_input *dev,
	unsigned int framerate);
extern int video_input_enum_framesize(const struct video_input *dev,
	struct v4l2_frmsizeenum *frmsizeenum);
extern int video_input_enum_frameintervals(const struct video_input *dev,
	struct v4l2_frmivalenum *frmivalenum);
extern int video_input_check_source_status(const struct video_input *dev);
extern int video_input_check_path_status(const struct video_input *dev);
extern int video_input_get_lastframe_addrs(const struct video_input *dev,
	unsigned int *addrs);
extern int video_input_create_lastframe(const struct video_input *dev);
extern int video_input_set_handover(const struct video_input *dev,
	int handover);
extern int video_input_set_lut(const struct video_input *dev,
	const struct lookup_table *l);
extern int video_input_poll(const struct video_input *dev);

extern int video_input_enum_frameinfo(const struct video_input *dev,
	unsigned int width, unsigned int height, unsigned int format);
extern int video_input_init_buffers(struct video_input *dev,
	unsigned int format, unsigned int io_mode);
extern int video_input_uninit_buffers(struct video_input *dev,
	unsigned int io_mode);
extern int video_input_start_preview(const struct video_input *dev);
extern int video_input_stop_preview(struct video_input *dev);

#endif//VIDEO_INPUT_H
