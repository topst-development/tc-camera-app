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
#include <errno.h>
#include <sys/mman.h>
#include <limits.h>
#include <stdbool.h>

#include "log.h"
#include "klog.h"
#include "v4l2.h"
#include "video_input.h"
#include "basic_operation.h"

int video_input_open_device(struct video_input *dev)
{
	int				ret		= 0;

	ret = v4l2_capture_open_device(&dev->capture);
	if (ret < 0) {
		loge("v4l2_capture_open_device, ret: %d\n", ret);
		ret = -1;
	}

	return ret;
}

int video_input_close_device(const struct video_input *dev)
{
	int				ret		= 0;

	ret = v4l2_capture_close_device(&dev->capture);
	if (ret < 0) {
		loge("v4l2_capture_close_device, ret: %d\n", ret);
		ret = -1;
	}

	return ret;
}

int video_input_query_capabilities(const struct video_input *dev)
{
	struct v4l2_capability		vid_cap;
	unsigned int			capabilities	= 0;
	int				ret		= 0;

	ret = v4l2_capture_query_capabilities(&dev->capture, &vid_cap);
	if (ret < 0) {
		loge("v4l2_capture_query_capabilities, ret: %d\n", ret);
		ret = -1;
	} else {
		logd("Driver: %s\n", vid_cap.driver);
		logd("Card: %s\n", vid_cap.card);
		logd("Version: %u\n", vid_cap.version);
		logd("Capabilities: 0x%x\n", (unsigned int)(vid_cap.capabilities));

		/* set capabilities to check */
		capabilities	= VIDEO_CAPTURE_CAP;

		if ((vid_cap.capabilities & capabilities) != capabilities) {
			loge("expected cap(0x%08x) != current cap(0x%08x)\n",
			     capabilities, vid_cap.capabilities);
			ret = -1;
		}
	}

	return ret;
}

int video_input_set_format(const struct video_input *dev,
	unsigned int width, unsigned int height, unsigned int format)
{
	struct v4l2_format		vid_fmt		= { 0, };
	struct v4l2_pix_format_mplane	*fmt		= NULL;
	unsigned int			idxpln		= 0;

	(void)memset((void *)&vid_fmt, 0,  (size_t)sizeof(struct v4l2_format));

	vid_fmt.type		= (unsigned int)VIDEO_CAPTURE_BUF_TYPE;

	fmt = &vid_fmt.fmt.pix_mp;
	fmt->width		= width;
	fmt->height		= height;
	fmt->pixelformat	= format;
	fmt->field		= v4l2_get_v4l2_field((unsigned int)format);
	fmt->num_planes		= u32_to_u8(v4l2_get_planes_by_v4l2_format(format));
	for (idxpln = 0U; idxpln < fmt->num_planes; idxpln++) {
		/* sizeimage */
		fmt->plane_fmt[idxpln].sizeimage	=
			v4l2_get_v4l2_sizeimage(format, width, height);
	}

	logd(" - width: %d\n", fmt->width);
	logd(" - height: %d\n", fmt->height);

	return v4l2_capture_set_format(&dev->capture, &vid_fmt);
}

int video_input_request_buffers(const struct video_input *dev,
	unsigned int mem, unsigned int count)
{
	struct v4l2_requestbuffers	req		= { 0, };
	int				ret		= 0;

	(void)memset(&req, 0,  sizeof(struct v4l2_requestbuffers));

	req.count	= count;
	req.type	= (unsigned int)VIDEO_CAPTURE_BUF_TYPE;
	req.memory	= mem;

	ret = v4l2_capture_reqbufs(&dev->capture, &req);
	if (ret < 0) {
		loge("[VIN %d] v4l2_capture_reqbufs, ret: %d\n",
			dev->capture.id, ret);
		ret = -1;
	} else {
		/* return the requested count */
		ret = u32_to_s32(req.count);
	}

	return ret;
}

int video_input_query_buffer(const struct video_input *dev,
	struct v4l2_buffer *vid_buf)
{
	int				ret		= 0;

	ret = v4l2_capture_querybuf(&dev->capture, vid_buf);
	if (ret < 0) {
		loge("[VIN %d] v4l2_capture_querybuf, ret: %d\n",
			dev->capture.id, ret);
		ret = -1;
	}

	return ret;
}

int video_input_expbuf(const struct video_input *dev,
	struct v4l2_exportbuffer *pbuf)
{
	int				ret		= 0;

	ret = v4l2_capture_expbuf(&dev->capture, pbuf);
	if (ret < 0) {
		logd("[VIN %d] v4l2_capture_expbuf, errno: %d\n",
			dev->capture.id, errno);
		ret = -1;
	}

	return ret;
}

int video_input_dqbuf(const struct video_input *dev, struct v4l2_buffer *pbuf)
{
	struct v4l2_buffer		buf		= { 0, };
	struct v4l2_plane		planes[VIDEO_MAX_PLANES];
	int				ret		= 0;

	(void)memset((void *)&buf, 0, sizeof(buf));
	(void)memset((void *)planes, 0, sizeof(planes));

	buf.type	= (unsigned int)VIDEO_CAPTURE_BUF_TYPE;
	buf.memory	= dev->io_mode;
	buf.m.planes	= planes;
	buf.length	= v4l2_get_planes_by_v4l2_format(dev->format);
	logd("type: %d, memory: %d, length: 0x%x\n",
		buf.type, buf.memory, buf.length);

	ret = v4l2_capture_dqbuf(&dev->capture, &buf);
	if (ret < 0) {
		logd("[VIN %d] v4l2_capture_dqbuf, errno: %d\n",
			dev->capture.id, errno);
		ret = -1;
	} else {
		/* return buf */
		*pbuf = buf;
	}

	return ret;
}

int video_input_qbuf(const struct video_input *dev, struct v4l2_buffer *pbuf)
{
	int				ret		= 0;

	ret = v4l2_capture_qbuf(&dev->capture, pbuf);
	if (ret < 0) {
		logd("[VIN %d] v4l2_capture_qbuf, errno: %d\n",
			dev->capture.id, errno);
		ret = -1;
	}

	return ret;
}

int video_input_start_stream(const struct video_input *dev)
{
	int				arg		= 0;
	int				ret		= 0;

	logk("> before video_input_start_stream");

	arg = (int)VIDEO_CAPTURE_BUF_TYPE;
	ret = v4l2_capture_streamon(&dev->capture, &arg);
	if (ret < 0) {
		loge("[VIN %d] v4l2_capture_streamon, ret: %d\n",
			dev->capture.id, ret);
		ret = -1;
	} else {
		/* wait until the video-input driver write some video data
		 * to the memory
		 */
		(void)usleep(10 * 1000);
	}

	logk("> after video_input_start_stream");

	return ret;
}

int video_input_stop_stream(const struct video_input *dev)
{
	int				arg		= 0;
	int				ret		= 0;

	logk("> before video_input_stop_stream");

	arg = (int)VIDEO_CAPTURE_BUF_TYPE;
	ret = v4l2_capture_streamoff(&dev->capture, &arg);
	if (ret < 0) {
		loge("[VIN %d] v4l2_capture_streamoff, ret: %d\n",
			dev->capture.id, ret);
		ret = -1;
	}

	logk("> after video_input_stop_stream");

	return ret;
}

int video_input_enum_input(const struct video_input *dev,
	struct v4l2_input *input)
{
	int				ret		= 0;

	ret = v4l2_capture_enuminput(&dev->capture, input);
	if (ret < 0) {
		loge("[VIN %d] v4l2_capture_enum_input, ret: %d\n",
			dev->capture.id, ret);
		ret = -1;
	}

	return ret;
}

int video_input_get_input(const struct video_input *dev,
	int *index)
{
	int				ret		= 0;

	ret = v4l2_capture_g_input(&dev->capture, index);
	if (ret < 0) {
		loge("[VIN %d] v4l2_capture_g_input, ret: %d\n",
			dev->capture.id, ret);
		ret = -1;
	}

	return ret;
}

int video_input_set_selection(const struct video_input *dev,
	unsigned int target, unsigned int flags, const struct rect *r)
{
	struct v4l2_selection		sel		= { 0, };
	int				ret		= 0;

	(void)memset((void *)&sel, 0, sizeof(sel));

	/**
	 * struct v4l2_selection - selection info
	 * @type:       buffer type (do not use *_MPLANE types)
	 */
	sel.type	= (int)V4L2_BUF_TYPE_VIDEO_CAPTURE;
	sel.target	= target;
	sel.flags	= flags;
	sel.r.left	= r->left;
	sel.r.top	= r->top;
	sel.r.width	= r->width;
	sel.r.height	= r->height;

	ret = v4l2_capture_s_selection(&dev->capture, &sel);
	if (ret < 0) {
		loge("[VIN %d] v4l2_capture_s_selection, ret: %d\n",
			dev->capture.id, ret);
		ret = -1;
	}

	return ret;
}

int video_input_get_framerate(const struct video_input *dev,
	unsigned int *framerate)
{
	struct v4l2_streamparm		parm		= { 0, };
	int				ret		= 0;

	(void)memset((void *)&parm, 0, sizeof(parm));

	parm.type = (unsigned int)VIDEO_CAPTURE_BUF_TYPE;
	ret = v4l2_capture_g_parm(&dev->capture, &parm);
	if (ret < 0) {
		loge("[VIN %d] v4l2_capture_g_parm, ret: %d\n",
			dev->capture.id, ret);
		ret = -1;
	} else {
		logd("type: 0x%08x\n", parm.type);
		logd("capture.capability: 0x%08x\n",
			parm.parm.capture.capability);
		logd("capture.timeperframe: %u / %u\n",
			parm.parm.capture.timeperframe.numerator,
			parm.parm.capture.timeperframe.denominator);

		*framerate = parm.parm.capture.timeperframe.denominator;
	}

	return ret;
}

int video_input_set_framerate(const struct video_input *dev,
	unsigned int framerate)
{
	struct v4l2_streamparm		parm		= { 0, };
	int				ret		= 0;

	(void)memset((void *)&parm, 0, sizeof(parm));

	parm.type = (unsigned int)VIDEO_CAPTURE_BUF_TYPE;
	ret = v4l2_capture_g_parm(&dev->capture, &parm);
	if (ret < 0) {
		loge("[VIN %d] v4l2_capture_g_parm, ret: %d\n",
			dev->capture.id, ret);
		ret = -1;
	} else {
		parm.parm.capture.capability			= V4L2_CAP_TIMEPERFRAME;
		parm.parm.capture.timeperframe.numerator	= 1;
		parm.parm.capture.timeperframe.denominator	= framerate;

		logd("type: 0x%08x\n", parm.type);
		logd("capture.capability: 0x%08x\n",
			parm.parm.capture.capability);
		logd("capture.timeperframe: %u / %u\n",
			parm.parm.capture.timeperframe.numerator,
			parm.parm.capture.timeperframe.denominator);

		ret = v4l2_capture_s_parm(&dev->capture, &parm);
		if (ret < 0) {
			loge("[VIN %d] v4l2_capture_s_parm, ret: %d\n",
				dev->capture.id, ret);
			ret = -1;
		}
	}

	return ret;
}

int video_input_enum_framesize(const struct video_input *dev,
	struct v4l2_frmsizeenum *frmsizeenum)
{
	int				ret		= 0;

	ret = v4l2_capture_enum_framesize(&dev->capture, frmsizeenum);
	if (ret < 0) {
		loge("[VIN %d] v4l2_capture_enum_framesize, ret: %d\n",
			dev->capture.id, ret);
		ret = -1;
	} else {
		logd("index: %d\n", frmsizeenum->index);
		logd("pixel_format: 0x%08x\n", frmsizeenum->pixel_format);
		logd("type: 0x%08x\n", frmsizeenum->type);
		logd("discrete.width: %d\n", frmsizeenum->discrete.width);
		logd("discrete.height: %d\n", frmsizeenum->discrete.height);
	}

	return ret;
}

int video_input_enum_frameintervals(const struct video_input *dev,
	struct v4l2_frmivalenum *frmivalenum)
{
	int				ret		= 0;

	ret = v4l2_capture_enum_frameintervals(&dev->capture, frmivalenum);
	if (ret < 0) {
		loge("[VIN %d] v4l2_capture_enum_frameintervals, ret: %d\n",
			dev->capture.id, ret);
		ret = -1;
	} else {
		logd("index: %d\n", frmivalenum->index);
		logd("pixel_format: 0x%08x\n", frmivalenum->pixel_format);
		logd("width: %d\n", frmivalenum->width);
		logd("height: %d\n", frmivalenum->height);
		logd("type: 0x%08x\n", frmivalenum->type);
		switch (frmivalenum->type) {
		case (unsigned int)V4L2_FRMIVAL_TYPE_DISCRETE:
			logd("discrete: %u / %u\n",
				frmivalenum->discrete.numerator,
				frmivalenum->discrete.denominator);
			break;
		default:
			loge("type is wrong\n");
			break;
		}
	}

	return ret;
}

static int video_input_get_input_with_index(const struct video_input *dev,
	struct v4l2_input *input)
{
	int				index		= 0;
	int				ret		= 0;

	ret = video_input_get_input(dev, &index);
	if (ret != 0) {
		loge("video_input_get_input, ret: %d\n", ret);
		ret = -1;
	} else {
		(void)memset(input, 0, sizeof(*input));
		input->index = s32_to_u32(index);
		ret = video_input_enum_input(dev, input);
		if (ret != 0) {
			loge("video_input_enum_input, ret: %d\n", ret);
			ret = -1;
		}
	}

	return ret;
}

int video_input_check_source_status(const struct video_input *dev)
{
	struct v4l2_input		input		= { 0, };
	int				status		= 0;
	int				ret		= 0;

	ret = video_input_get_input_with_index(dev, &input);
	if (ret < 0) {
		logd("Failed to get v4l2 input instance with index\n");
		if (ret == -ENOTTY) {
			/* not supported */
			logw("video.g_input_status is not supported\n");
		}
		status = 1;
	} else {
		logd("video_input_enum_input, status: 0x%08x\n", input.status);
		if ((input.status & (unsigned int)V4L2_IN_ST_NO_POWER) != 0U) {
			/* status is unstable */
			loge("video_input_enum_input, status: 0x%08x\n",
				input.status);
		} else {
			/* status is stable */
			status = 1;
		}
	}

	return status;
}

int video_input_check_path_status(const struct video_input *dev)
{
	int				status		= 0;
	int				ret		= 0;

	logk("> before video_input_check_path_status");

	ret = v4l2_capture_check_v4l2_dev_status(&dev->capture, &status);
	if (ret < 0) {
		logd("[VIN %d] v4l2_capture_check_v4l2_dev_status, ret: %d\n",
			dev->capture.id, ret);
		ret = -1;
	} else {
		if (status <= 0) {
			/* unstable */
			logd("[VIN %d] The video-input path is NOT working\n",
				dev->capture.id);
		}
		ret = status;
	}

	logk("> after video_input_check_path_status");

	return ret;
}

int video_input_get_lastframe_addrs(const struct video_input *dev,
	unsigned int *addrs)
{
	int				ret		= 0;

	ret = v4l2_capture_g_lastframe_addrs(&dev->capture, addrs);
	if (ret < 0) {
		loge("[VIN %d] v4l2_capture_s_handover, ret: %d\n",
			dev->capture.id, ret);
		ret = -1;
	}

	return ret;
}

int video_input_create_lastframe(const struct video_input *dev)
{
	unsigned int			addrs		= 0;
	int				ret		= 0;

	ret = v4l2_capture_create_lastframe(&dev->capture, &addrs);
	if (ret < 0) {
		loge("[VIN %d] v4l2_capture_create_lastframe, ret: %d\n",
			dev->capture.id, ret);
		ret = -1;
	}

	return ret;
}

int video_input_set_handover(const struct video_input *dev,
	int handover)
{
	int				ret		= 0;

	ret = v4l2_capture_s_handover(&dev->capture, &handover);
	if (ret < 0) {
		loge("[VIN %d] v4l2_capture_s_handover, ret: %d\n",
			dev->capture.id, ret);
		ret = -1;
	}

	return ret;
}

int video_input_set_lut(const struct video_input *dev,
	const struct lookup_table *l)
{
	struct vin_lut			lut		= { 0, };
	const unsigned char		*src		= NULL;
	unsigned char			*dst		= NULL;
	int				ret		= 0;

	src = *l->table;
	dst = (unsigned char *)lut.lut;

	(void)memcpy(dst, src, sizeof(unsigned char) * 256U * 3U);

	ret = v4l2_capture_s_lut(&dev->capture, &lut);
	if (ret < 0) {
		loge("[VIN %d] v4l2_capture_s_lut, ret: %d\n",
			dev->capture.id, ret);
		ret = -1;
	}

	return ret;
}

int video_input_poll(const struct video_input *dev)
{
	short				events		= POLLIN;
	int				timeout		= 50;		// 50ms
	int				ret		= 0;

	ret = v4l2_capture_poll(&dev->capture, events, timeout);
	if (ret == -1) {
		loge("v4l2_capture_poll, ret: %d\n", ret);
		ret = -1;
	} else if (ret == 0) {
		/* timeout */
		logd("v4l2_capture_poll, ret: %d (timeout)\n", ret);
	} else {
		/* check revents */
		if (((unsigned int)ret & s16_to_u32(events)) != s16_to_u32(events)) {
			/* events != revents */
			logd("events (0x%x) != revents (0x%x)\n",
				(unsigned int)events, (unsigned int)ret);
		}
	}

	return ret;
}

static int enum_frminfo_size(const struct video_input *dev,
	unsigned int width, unsigned int height, unsigned int format)
{
	struct v4l2_frmsizeenum		frmsizeenum;
	struct v4l2_frmsizeenum		frmsizeenum_max;
	int				ret		= 0;
	int				index		= 0;

	(void)memset(&frmsizeenum, 0, sizeof(frmsizeenum));
	(void)memset(&frmsizeenum_max, 0, sizeof(frmsizeenum_max));

	frmsizeenum.pixel_format	= format;
	frmsizeenum.type		= (unsigned int)V4L2_FRMSIZE_TYPE_DISCRETE;

	index = 0;
	do {
		frmsizeenum.index = s32_to_u32(index);
		ret = video_input_enum_framesize(dev, &frmsizeenum);
		if (ret < 0) {
			loge("enum framesize, ret: %d, no framesize is supported\n", ret);
			ret = -1;
		} else {
			if ((frmsizeenum.discrete.width == width) &&
			    (frmsizeenum.discrete.height == height)) {
				logd("framesize (%d * %d) is supported\n",
					frmsizeenum.discrete.width,
					frmsizeenum.discrete.height);
				frmsizeenum_max = frmsizeenum;
				break;
			}

			if (index < INT_MAX) {
				/* increase index to query */
				index++;
			}
		}
	} while (ret == 0);

	if ((frmsizeenum_max.discrete.width  != 0U) &&
	    (frmsizeenum_max.discrete.height != 0U)) {
		/* print */
		logd("selected framesize is %d * %d\n",
			frmsizeenum.discrete.width,
			frmsizeenum.discrete.height);
	}

	return ret;
}

static int enum_frminfo_interval(const struct video_input *dev,
	unsigned int width, unsigned int height, unsigned int format)
{
	struct v4l2_frmivalenum		frmivalenum;
	struct v4l2_frmivalenum		frmivalenum_max;
	int				ret		= 0;
	unsigned int			numerator	= 1;
	unsigned int			denominator	= 60;
	unsigned int			index		= 0;

	(void)memset(&frmivalenum, 0, sizeof(frmivalenum));
	(void)memset(&frmivalenum_max, 0, sizeof(frmivalenum_max));

	frmivalenum.pixel_format	= format;
	frmivalenum.width		= width;
	frmivalenum.height		= height;
	frmivalenum.type		= (unsigned int)V4L2_FRMSIZE_TYPE_DISCRETE;

	index = 0U;
	do {
		frmivalenum.index = index;
		ret = video_input_enum_frameintervals(dev, &frmivalenum);
		if (ret < 0) {
			loge("enum framerate, ret: %d, no framerate is supported\n", ret);
			ret = -1;
		} else {
			if ((frmivalenum.discrete.numerator   == (unsigned int)numerator) &&
			    (frmivalenum.discrete.denominator == (unsigned int)denominator)) {
				logd("frmivalenum (%u / %u) is supported\n",
					frmivalenum.discrete.numerator,
					frmivalenum.discrete.denominator);
				frmivalenum_max = frmivalenum;
				break;
			}

			/* coverity[misra_c_2012_rule_12_1_violation : FALSE] */
			if (index < (unsigned int)INT_MAX) {
				/* increase index to query */
				index++;
			}
		}
	} while (ret == 0);

	if ((frmivalenum_max.discrete.numerator   != 0U) &&
	    (frmivalenum_max.discrete.denominator != 0U)) {
		/* print */
		logd("selected frmivalenum is %u / %u\n",
			frmivalenum.discrete.numerator,
			frmivalenum.discrete.denominator);
	}

	return ret;
}

int video_input_enum_frameinfo(const struct video_input *dev,
	unsigned int width, unsigned int height, unsigned int format)
{
	int				ret		= 0;

	ret = enum_frminfo_size(dev, width, height, format);
	if (ret < 0) {
		loge("enum_frminfo_size, ret: %d\n", ret);
		ret = -1;
	} else {
		ret = enum_frminfo_interval(dev, width, height, format);
		if (ret < 0) {
			loge("enum_frminfo_interval, ret: %d\n", ret);
			ret = -1;
		}
	}

	return ret;
}

static int video_input_allocate_buffers(struct video_input *dev,
	unsigned int io_mode)
{
	int				ret		= 0;

	ret = video_input_request_buffers(dev, io_mode, NUM_VIDBUF);
	if (ret <= 0) {
		loge("video_input_request_buffers, ret: %d\n", ret);
		ret = -1;
	} else {
		dev->n_allocated_buf = s32_to_u32(ret);
		logd("The number of the allocated buffer: %d\n",
		     dev->n_allocated_buf);

		/* coverity[misra_c_2012_rule_11_5_violation : FALSE] */
		/* coverity[misra_c_2012_rule_21_3_violation : FALSE] */
		dev->buffers = (struct buffer_t *)malloc(sizeof(struct buffer_t) * dev->n_allocated_buf);
		if (dev->buffers == NULL) {
			loge("allocate buffer memory\n");
			ret = -1;
		}
	}

	return ret;
}

static int video_input_init_plane_addr(const struct video_input *dev,
	struct buf_addr *paddrs, struct buf_addr *vaddrs,
	const struct v4l2_plane *pln)
{
	int				ret		= 0;

	/* coverity[misra_c_2012_rule_10_1_violation : FALSE] */
	vaddrs->addr = (void *)mmap(NULL, pln->length, PROT_READ | PROT_WRITE, MAP_SHARED,
				dev->capture.fd, (off_t)pln->m.mem_offset);

	logd("vaddrs->addr = %p\n", vaddrs->addr);

	/* coverity[misra_c_2012_rule_11_6_violation : FALSE] */
	/* coverity[cert_int36_c_violation : FALSE] */
	if (vaddrs->addr == MAP_FAILED) {
		/* If you do not exit here you should unmap() and free()
		 * the buffers mapped so far.
		 */
		loge("mmap");
		ret = -1;
	} else {
		vaddrs->length	= pln->length;
		/* coverity[misra_c_2012_rule_11_6_violation : FALSE] */
		/* coverity[cert_int36_c_violation : FALSE] */
		paddrs->addr	= pln->reserved[0];
		paddrs->length	= pln->length;
	}

	return ret;
}

static void video_input_init_vidbuf(struct v4l2_buffer *vid_buf,
	unsigned int format, unsigned int io_mode, unsigned int idxBuf, struct v4l2_plane *planes)
{
	vid_buf->index		= idxBuf;
	vid_buf->type		= (unsigned int)VIDEO_CAPTURE_BUF_TYPE;
	vid_buf->memory		= io_mode;
	vid_buf->m.planes	= planes;
	vid_buf->length		= v4l2_get_planes_by_v4l2_format(format);
	logd("num_planes: %d\n", vid_buf->length);

}

static int video_input_init_buf_addr(const struct video_input *dev,
	unsigned int format, unsigned int io_mode, unsigned int idxBuf)
{
	struct v4l2_buffer		vid_buf		= { 0, };
	struct v4l2_plane		planes[VIDEO_MAX_PLANES];
	struct buf_addr			*vaddrs		= NULL;
	struct buf_addr			*paddrs		= NULL;
	unsigned int			idxpln		= 0;
	int				ret		= 0;

	vaddrs = dev->buffers[idxBuf].vaddrs;
	paddrs = dev->buffers[idxBuf].paddrs;

	(void)memset(&vid_buf, 0, sizeof(vid_buf));
	(void)memset(&planes, 0,  sizeof(planes));
	(void)memset(vaddrs, 0, sizeof(struct buf_addr) * (unsigned int)VIDEO_MAX_PLANES);
	(void)memset(paddrs, 0, sizeof(struct buf_addr) * (unsigned int)VIDEO_MAX_PLANES);

	video_input_init_vidbuf(&vid_buf, format, io_mode, idxBuf, planes);

	ret = video_input_query_buffer(dev, &vid_buf);
	if (ret < 0) {
		loge("video_input_query_buffer, ret: %d\n", ret);
		ret = -1;
	} else {
		for (idxpln = 0; idxpln < vid_buf.length; idxpln++) {
			ret = video_input_init_plane_addr(dev, &paddrs[idxpln],
							  &vaddrs[idxpln], &planes[idxpln]);
			if (ret < 0) {
				/* error */
				loge("video_input_init_plane_addr, ret: %d\n", ret);
			}
		}

		ret = video_input_qbuf(dev, &vid_buf);
		if (ret < 0) {
			loge("[VIN %d] video_input_qbuf, ret: %d\n",
				dev->capture.id, ret);
			ret = -1;
		}
	}

	return ret;
}

int video_input_init_buffers(struct video_input *dev,
				 unsigned int format, unsigned int io_mode)
{
	unsigned int			idxBuf		= 0;
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else if (io_mode != (unsigned int)V4L2_MEMORY_MMAP) {
		loge("memory type (0x%08x) is not supported\n", io_mode);
		ret = -1;
	} else {
		logd("width: %d, height: %d\n", dev->frame_width, dev->frame_height);

		INIT_LIST_HEAD(&dev->buf_list);

		ret = video_input_allocate_buffers(dev, io_mode);
		if (ret < 0) {
			loge("Failed to allocate buffers\n");
			ret = -1;
		} else {
			for (idxBuf = 0U;
			     idxBuf < dev->n_allocated_buf; idxBuf++) {
				ret = video_input_init_buf_addr(dev, format, io_mode, idxBuf);
				if (ret < 0) {
					/* error */
					break;
				} else {
					/* pass */
					logd("Succeed to initialize buffer address\n");
				}
			}
		}
	}

	return ret;
}

static int video_input_deallocate_buffers(struct video_input *dev,
					   unsigned int io_mode)
{
	unsigned int			idxBuf		= 0;
	int				idxpln		= 0;
	int				ret		= 0;

	for (idxBuf = 0; idxBuf < dev->n_allocated_buf; idxBuf++) {
		for (idxpln = 0; idxpln < VIDEO_MAX_PLANES; idxpln++) {
			if (dev->buffers[idxBuf].vaddrs[idxpln].addr != NULL) {
				(void)munmap(dev->buffers[idxBuf].vaddrs[idxpln].addr,
					dev->buffers[idxBuf].vaddrs[idxpln].length);
				dev->buffers[idxBuf].vaddrs[idxpln].addr = NULL;
			}
		}
	}

	ret = video_input_request_buffers(dev, io_mode, 0);
	if (ret < 0) {
		loge("video_input_request_buffers, ret: %d\n", ret);
		ret = -1;
	} else {
		dev->n_allocated_buf = s32_to_u32(ret);
		logd("The number of the allocated buffer: %d\n",
			dev->n_allocated_buf);
		/* coverity[misra_c_2012_rule_21_3_violation : FALSE] */
		free(dev->buffers);
	}

	return ret;
}

int video_input_uninit_buffers(struct video_input *dev,
	unsigned int io_mode)
{
	int				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else if (io_mode != (unsigned int)V4L2_MEMORY_MMAP) {
		loge("memory type (0x%08x) is not supported\n", io_mode);
		ret = -1;
	} else {
		/* de-allocate the ion buffers */
		ret = video_input_deallocate_buffers(dev, io_mode);
	}

	return ret;
}

static int start_preview_init_format(const struct video_input *dev)
{
	unsigned int			fmt		= 0;
	int				ret		= 0;

	fmt = dev->format;
	ret = video_input_set_format(dev, dev->frame_width, dev->frame_height, fmt);
	if (ret < 0) {
		loge("video_input_set_format, ret: %d\n", ret);
		ret = -1;
	}

	return ret;
}

static int start_preview_init_framerate(struct video_input *dev)
{
	unsigned int			frmrate		= 0;
	int				ret		= 0;

	/* init fraerate */
	frmrate = dev->framerate;

	ret = video_input_set_framerate(dev, frmrate);
	if (ret < 0) {
		loge("video_input_set_parm, ret: %d\n", ret);
		ret = -1;
	} else {
		ret = video_input_get_framerate(dev, &frmrate);
		if (ret < 0) {
			loge("video_input_get_parm, ret: %d\n", ret);
			/* return ret; */
		} else {
			/* pass */
			dev->framerate = frmrate;
		}
	}

	return ret;
}

static int start_preview_init_buffers(const struct video_input *dev)
{
	int				ret		= 0;

	/* coverity[misra_c_2012_rule_11_8_violation : FALSE] */
	/* coverity[cert_exp40_c_violation : FALSE] */
	ret = video_input_init_buffers(dev, dev->format, dev->io_mode);
	if (ret < 0) {
		loge("video_input_init_buffers, ret: %d\n", ret);
		ret = -1;
	}

	return ret;
}

static int start_preview_init_selection(const struct video_input *dev)
{
	int				ret		= 0;

	/* crop */
	ret = video_input_set_selection(dev, V4L2_SEL_TGT_CROP, 0, &dev->crop);

	/* compose */
	ret = video_input_set_selection(dev, V4L2_SEL_TGT_COMPOSE, dev->comp_flags, &dev->comp);

	return ret;
}

static int start_preview_enable_stream(const struct video_input *dev)
{
	int				ret		= 0;

	ret = video_input_start_stream(dev);
	if (ret != 0) {
		loge("video_input_start_stream, ret: %d\n", ret);
		ret = -1;
	}

	return ret;
}

static int (*func_to_start_preview[START_PREVIEW_STEPS_MAX])(
	const struct video_input *dev) = {
	start_preview_init_format,
	/* coverity[misra_c_2012_rule_11_1_violation : FALSE] */
	start_preview_init_framerate,
	start_preview_init_buffers,
	start_preview_init_selection,
	start_preview_enable_stream,
};

int video_input_start_preview(const struct video_input *dev)
{
	int				idx		= 0;
	int				ret		= 0;

	for (idx = 0; idx < (int)START_PREVIEW_STEPS_MAX; idx++) {
		ret = func_to_start_preview[idx](dev);
		if (ret < 0) {
			loge("Failed to start stream(%dth function)\n", idx);
			break;
		}
	}

	return ret;
}

int video_input_stop_preview(struct video_input *dev)
{
	int				ret		= 0;

	ret = video_input_stop_stream(dev);
	if (ret != 0) {
		loge("video_input_stop_stream, ret: %d\n", ret);
	}

	ret = video_input_uninit_buffers(dev, dev->io_mode);
	if (ret != 0) {
		/* error */
		loge("video_input_uninit_buffers, ret: %d\n", ret);
	}

	return ret;
}
