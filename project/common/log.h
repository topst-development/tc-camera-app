/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) Telechips Inc.
 */

#ifndef LOG_H
#define LOG_H

#define APP_MODULE_NAME		("RCAM")

#define log_l(level, msg...)	{						\
	/* coverity[misra_c_2012_rule_21_6_violation : FALSE] */		\
	(void)printf("[%s][%s] %s - ", level, APP_MODULE_NAME, __func__);	\
	/* coverity[misra_c_2012_rule_21_6_violation : FALSE] */		\
	(void)printf(msg);							\
}

#if 0
#define loge(msg...)		{ log_l("ERROR",	msg) }
#define logw(msg...)		{ log_l("WARNING",	msg) }
#define logi(msg...)		{ log_l("INFO",	msg) }
#define logd(msg...)		//{ log_l("DEBUG",	msg) }
#else
#define loge(msg...)
#define logw(msg...)
#define logi(msg...)
#define logd(msg...)
#endif

#endif//LOG_H
