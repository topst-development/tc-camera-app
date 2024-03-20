// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Telechips Inc.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <pthread.h>
/* coverity[misra_c_2012_rule_21_5_violation : FALSE] */
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "log.h"
#include "klog.h"
#include "v4l2.h"
#include "switch.h"
#include "cm4_manager.h"
#include "camera.h"
#include "basic_operation.h"

static struct camera	g_dev;
static int32_t		is_suspended;
static pthread_t	hndThread;

static void help_msg(void)
{
	logi("\n\n"
		"# Usage: camera_app {options}\n"
		"\n"
		" --switch={decimal}: reverse switch device number\n"
		"  . ex) --switch=0 or --switch=1\n"
		" --videoinput={decimal}: video-input path(v4l2 capture) device number\n"
		"  . ex) --videoinput=0\n"
		" --capture={decimal}: set the number of frames to write into files\n"
		"  . ex) --capture=3\n"
		" --compose_flags={decimal}: composition flag (refer to v4l2 selection flag)\n"
		"  . ex) --compose_flags=0\n"
		" --compose_posx={decimal}: x axis of composition\n"
		"  . ex) --compose_posx=100\n"
		" --compose_posy={decimal}: y axis of composition\n"
		"  . ex) --compose_posy=100\n"
		" --compose_width={decimal}: width of composition\n"
		"  . ex) --compose_width=640\n"
		" --compose_height={decimal}: height of composition\n"
		"  . ex) --compose_height=480\n"
		" --crop_posx={decimal}: x axis of crop\n"
		"  . ex) --crop_posx=100\n"
		" --crop_posy={decimal}: y axis of crop\n"
		"  . ex) --crop_posy=100\n"
		" --crop_width={decimal}: width of crop\n"
		"  . ex) --crop_width=640\n"
		" --crop_height={decimal}: height of crop\n"
		"  . ex) --crop_height=480\n"
		" --preview_posx={decimal}: preview position x, -1 is horizontal center\n"
		"  . ex) --preview_posx=100\n"
		" --preview_posy={decimal}: preview position y, -1 is vertical center\n"
		"  . ex) --preview_posy=100\n"
		" --preview_width={decimal}: preview width (default is full screen size)\n"
		"  . ex) --preview_width=640\n"
		" --preview_height={decimal}: preview height (default is full screen size)\n"
		"  . ex) --preview_height=480\n"
		" --preview_format={string}: preview format\n"
		"  . ex) --preview_format=rgb32\n"
		" --preview_method={decimal}: preview method (supported in the telechips's customized v4l2 standard)\n"
		"  . options\n"
		"   + 0: v4l2 buffer switching method\n"
		"   + 1: direct display method\n"
		"  . ex) --preview_method=0\n"
		" --preview_rot={decimal}: preview rotation (supported in v4l2 buffer switching mode)\n"
		"  . options\n"
		"   + degree: 0, 90, 180, 270\n"
		"  . ex) --preview_rot=90\n"
		" --io-mode={string}: v4l2 memory allocation method\n"
		"  . options\n"
		"   + mmap: memory allocated in v4l2-capture driver\n"
		"   + userptr: memory allocated in user space\n"
		"  . ex) --io-mode=0 or --io-mode=1\n"
		" --videooutput={decimal}: display-output path device number\n"
		"  . ex) --videooutput=0\n"
		" --foreground_ovp={decimal}: foreground ovp of display-output path's wmixer\n"
		"  . ex) --foreground_ovp=16\n"
		" --background_ovp={decimal}: background ovp of display-output path's wmixer\n"
		"  . ex) --background_ovp=28\n"
		" --ignore_ovp={decimal}: ignore to change ovp of display-output path's wmixer\n"
		"  . options\n"
		"   + 0: change ovp whenever starting and stopping preview\n"
		"   + 1: ignore to change ovp\n"
		"  . ex) --ignore_ovp=1\n"
		" --recovery={decimal}: ignore to recovery video-input path\n"
		"  . options\n"
		"   + 0: ignore to recovery\n"
		"   + 1: recovery video-input path if it's not working\n"
		"  . ex) --recovery=0\n"
		" --use_cm4={decimal}: ignore to recovery video-input path\n"
		"  . options\n"
		"   + 0: use not cm4\n"
		"   + 1: use cm4 for early camera\n"
		"  . ex) --use_cm4=1\n"
		" --boot_profile={decimal}: show boot profile\n"
		"  . options\n"
		"   + 0: show boot profile\n"
		"   + 1: hide boot profile\n"
		"  . ex) --boot_profile=1\n"
		"\n\n");
}

/*
 * According to MISRA2012 ruleset, we need to avoid dynamic memory allocation
 * using heap.
 */
#define NUM_OPTIONS 30

/*
 * According to MISRA2012 ruleset, the object pointer must be matched or cast,
 * but the function is excluded.
 */
static int32_t parse_args(int32_t argc, char * const argv[], struct camera *dev)
{
	const struct option		long_options[NUM_OPTIONS] = {
		// long-name,		name,			variable,			short-name or init-val
		{"switch",		required_argument,	&dev->sw.id,			0},
		{"videoinput",		required_argument,	&dev->vin.capture.id,		0},
		{"capture",		required_argument,	&dev->cnt_to_capture,		0},
		{"compose_flags",	required_argument,	&dev->vin.comp_flags,		0},
		{"compose_posx",	required_argument,	&dev->vin.comp.left,		0},
		{"compose_posy",	required_argument,	&dev->vin.comp.top,		0},
		{"compose_width",	required_argument,	&dev->vin.comp.width,		0},
		{"compose_height",	required_argument,	&dev->vin.comp.height,		0},
		{"crop_posx",		required_argument,	&dev->vin.crop.left,		0},
		{"crop_posy",		required_argument,	&dev->vin.crop.top,		0},
		{"crop_width",		required_argument,	&dev->vin.crop.width,		0},
		{"crop_height",		required_argument,	&dev->vin.crop.height,		0},
		{"io-mode",		required_argument,	&dev->vin.io_mode,		0},
		{"preview_posx",	required_argument,	&dev->preview_posx,		0},
		{"preview_posy",	required_argument,	&dev->preview_posy,		0},
		{"preview_width",	required_argument,	&dev->preview_width,		0},
		{"preview_height",	required_argument,	&dev->preview_height,		0},
		{"preview_format",	required_argument,	&dev->preview_format,		0},
		{"preview_method",	required_argument,	&dev->preview_method,		0},
		{"preview_rot",		required_argument,	&dev->preview_rot,		0},
		{"videooutput",		required_argument,	&dev->vout.ovl.id,		0},
		{"application_mode",	required_argument,	&dev->application_mode,		0},
		{"foreground_ovp",	required_argument,	&dev->vout.ovl.wmix_fovp,	0},
		{"background_ovp",	required_argument,	&dev->vout.ovl.wmix_bovp,	0},
		{"ignore_ovp",		required_argument,	&dev->vout.ignore_ovp,		0},
		{"recovery",		required_argument,	&dev->recovery,			0},
		{"use_cm4",		required_argument,	&dev->use_cm4,			0},
		{"boot_profile",	required_argument,	&g_log_level,			0},
		{NULL,			0,			NULL,				0},
		{NULL,			0,			NULL,				0},
	};
	int32_t				c               = 0;
	int32_t				option_index	= -1;
	int32_t				ret		= 0;

	while (true) {
		c = getopt_long(argc, argv, "", long_options, &option_index);
		if ((c == -1) || (c == (int32_t)'?')) {
			if (c == (int32_t)'?') {
				/* coverity[misra_c_2012_rule_2_2_violation : FALSE] */
				help_msg();
				ret = -1;	
			}
			break;
		} else {
			uint32_t u_opt_idx = s32_to_u32(option_index);
			if (strcmp(long_options[u_opt_idx].name, "preview_format") == 0) {
				*long_options[u_opt_idx].flag = u32_to_s32(v4l2_get_v4l2_format_by_name(optarg));
			} else {
				/* coverity[cert_err34_c_violation : FALSE] */
				/* coverity[misra_c_2012_rule_21_7_violation : FALSE] */
				*long_options[u_opt_idx].flag = atoi(optarg);
			}
		}
	}

#if defined(USE_G2D)
	/*
	 *	In case of 90 or 270 rotation
	 *		rotation_ratio:	preview_height * 100 / preview_width
	 *		rotated_width:	preview_width  * rotation_ratio / 100
	 *		rotated_height:	preview_height * rotation_ratio / 100
	 */
	int32_t	rotation_ratio	= 0;

	// get rotation ratio
	switch (dev->preview_rot) {
	case 0:
		dev->preview_rot	= (uint32_t)NOOP;
		break;
	case 90:
		dev->preview_rot	= (uint32_t)ROTATE_90;
		rotation_ratio		= dev->preview_height * 100 / dev->preview_width;
		dev->preview_width	= dev->preview_width  * rotation_ratio / 100;
		dev->preview_height	= dev->preview_height * rotation_ratio / 100;
		break;
	case 180:
		dev->preview_rot	= (uint32_t)ROTATE_180;
		break;
	case 270:
		dev->preview_rot	= (uint32_t)ROTATE_270;
		rotation_ratio		= dev->preview_height * 100 / dev->preview_width;
		dev->preview_width	= dev->preview_width  * rotation_ratio / 100;
		dev->preview_height	= dev->preview_height * rotation_ratio / 100;
		break;
	default:
		loge("preview_rot(%u) is wrong\n", dev->preview_rot);
		ret = -1;
		break;
	}
#endif//defined(USE_G2D)

	return ret;
}

static void doOnStatus(const void *data)
{
	/* coverity[misra_c_2012_rule_11_5_violation : FALSE] */
	const struct camera		*dev		=
		(const struct camera *)data;
	int32_t				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		ret = camera_start_preview(dev);
	}

	if (ret < 0) {
		loge("Failed on doOnStatus\n");
	}
}

static void doOffStatus(const void *data)
{
	/* coverity[misra_c_2012_rule_11_5_violation : FALSE] */
	const struct camera		*dev		=
		(const struct camera *)data;
	int32_t				ret		= 0;

	if (dev == NULL) {
		loge("dev is NULL\n");
		ret = -1;
	} else {
		ret = camera_stop_preview(dev);
	}

	if (ret < 0) {
		loge("Failed on doOffStatus\n");
	}
}

static void signal_handler(int sig)
{
	/* coverity[cert_sig31_c_violation : FALSE] */
	const struct camera		*dev		= &g_dev;
	int32_t				ret;

	logi("Get Signal %d\n", sig);
	switch (sig) {
	case SIGTSTP:
		logi("SIGTSTP\n");
		is_suspended = 1;
		/* coverity[cert_sig30_c_violation : FALSE] */
		ret = camera_stop_preview(dev);
		if (ret != 0) {
			loge("Failed on camera_stop_preview\n");
		}
		/* coverity[cert_sig30_c_violation : FALSE] */
		ret = raise(SIGSTOP);
		if (ret != 0) {
			loge("Failed on raise SIGSTOP\n");
		}
		break;
	case SIGCONT:
		is_suspended = 0;
		break;
	default:
		loge("signal is wrong\n");
		break;
	}
}

static int32_t process_command(const void *data, const char *cmdline)
{
	int32_t				ret		= 0;

	if (strncmp("start", cmdline, 5) == 0) {
		doOnStatus(data);
	} else if (strncmp("stop", cmdline, 4) == 0) {
		doOffStatus(data);
	} else if (strncmp("quit", cmdline, 4) == 0) {
		doOffStatus(data);
		ret = -1;
	} else {
		loge("invalid input\n\n");
	}

	return ret;
}

/* coverity[misra_c_2012_rule_8_13_violation : FALSE] */
static void cleanup_switch_manager(void *data)
{
	char		cmdline[1024]	= "";
	int32_t		ret		= 0;

	while (true) {
		/*
		 * clear the command string
		 */
		(void)memset(cmdline, 0, sizeof(cmdline));

		/*
		 * read a command
		 */
		ret = s64_to_s32(read(STDIN_FILENO, cmdline, sizeof(cmdline)));
		if (ret == 0) {
			continue;
		}

		ret = process_command(data, cmdline);
		if (ret < 0) {
			/* coverity[misra_c_2012_rule_21_6_violation : FALSE] */
			logi("terminated\n");
			break;
		}

		(void)usleep(100 * 1000);
	}
}

/* coverity[misra_c_2012_rule_8_13_violation : FALSE] */
static void detect_switch_changed(const struct camera *dev, void *data)
{
	int32_t				sw_prev		= -1;
	int32_t				sw_curr		= -1;

	while (true) {
		if (is_suspended == 1) {
			/* coverity[assigned_value : FALSE] */
			sw_prev = -1;
			sw_curr = -1;
		} else {
			sw_prev = sw_curr;
			sw_curr = switch_get_state(&dev->sw);
			if (sw_prev != sw_curr) {
				if (sw_curr > 0) {
					logd("sw state: %d -> %d\n",
					     sw_prev, sw_curr);
					doOnStatus(data);
				} else {
					logd("sw state: %d -> %d\n",
					     sw_prev, sw_curr);
					doOffStatus(data);
				}
			}
		}
		(void)usleep(100 * 1000);
	}
}

static void *threadSwitchManager(void *data)
{
	/* coverity[misra_c_2012_rule_11_5_violation : FALSE] */
	const struct camera		*dev		=
		(const struct camera *)data;


	if ((dev == NULL) || (dev->sw.id < 0)) {
		cleanup_switch_manager(data);
	} else {
		detect_switch_changed(dev, data);
	}

	return NULL;
}

static int32_t app_initialize(struct camera *dev)
{
	int32_t				ret		= 0;

	// open devices
	ret = camera_open_devices(dev);
	if (ret < 0) {
		/* coverity[misra_c_2012_rule_21_6_violation : FALSE] */
		loge("open, ret: %d\n", ret);
		camera_close_devices(dev);
	} else {
		// show parameters
		camera_show_parameters(dev);

		// create camera threads
		ret = camera_create_camera_thread(dev);
		if (ret < 0) {
			/* coverity[misra_c_2012_rule_21_6_violation : FALSE] */
			loge("create message thread, ret: %d\n", ret);
			ret = camera_destroy_camera_thread(dev);
		} else {
			// handover
			ret = camera_handover(dev);
		}
	}

	return ret;
}

/* coverity[misra_c_2012_rule_8_13_violation : FALSE] */
static int32_t app_reg_sighandler(struct camera *dev)
{
	int32_t				ret		= 0;

	(void)dev;

	// register signal handler
	/* coverity[cert_int36_c_violation : FALSE] */
	/* coverity[misra_c_2012_rule_11_1_violation : FALSE] */
	if (signal(SIGTSTP, &signal_handler) == SIG_ERR) {
		/* coverity[misra_c_2012_rule_21_6_violation : FALSE] */
		loge("Failed on signal(SIGTSTP)\n");
	}

	/* coverity[cert_int36_c_violation : FALSE] */
	if (signal(SIGCONT, &signal_handler) == SIG_ERR) {
		/* coverity[misra_c_2012_rule_21_6_violation : FALSE] */
		loge("Failed on signal(SIGCONT)\n");
	}


	return ret;
}

static int32_t app_finalize(struct camera *dev)
{
	int32_t				ret		= 0;

	// create & start
	ret = pthread_create(&hndThread, NULL, &threadSwitchManager,
			     (void *)dev);
	if (ret < 0) {
		/* coverity[misra_c_2012_rule_21_6_violation : FALSE] */
		perror("ERROR: pthread_create");
	} else {
		// stop & terminate
		ret = pthread_join(hndThread, NULL);
		if (ret != 0) {
			perror("ERROR: failed on pthread_join\n");
		}
	}

	return ret;
}

#define NUM_INIT_FUNC	3

static int32_t (*init_funcs[NUM_INIT_FUNC])(struct camera *dev) = {
	app_initialize,
	app_reg_sighandler,
	app_finalize
};

int32_t main(int32_t argc, char * const argv[])
{
	struct camera			*dev		= NULL;
	int32_t				ret		= 0;
	int32_t				idx		= 0;

	logk("> rvc app started");

	logi("## CAMERA APPLICATION ##\n");

	dev = &g_dev;

	camera_init_parameters(dev);

	ret = parse_args(argc, argv, dev);
	if (ret == 0) {
		for (idx = 0; idx < NUM_INIT_FUNC; idx++) {
			ret = init_funcs[idx](dev);
			if (ret != 0) {
				break;
			}
		}
	}

	logk("> rvc app stopped");

	return ret;
}
