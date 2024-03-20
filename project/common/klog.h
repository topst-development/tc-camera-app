/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) Telechips Inc.
 */

#ifndef KLOG_H
#define KLOG_H

#define	logk			(void)klog_printl

extern int g_log_level;

extern int klog_printl(const char *msg);

#endif//KLOG_H
