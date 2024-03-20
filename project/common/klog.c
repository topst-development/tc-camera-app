// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Telechips Inc.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>

#include "klog.h"
#include "log.h"
#include "basic_operation.h"

int	g_log_level		= 0;

static int print_to_kmsg_node(const char *msg)
{
	pid_t				child		= { 0, };
	int				fd		= 0;
	int				ret		= 0;

	child = fork();
	if (child == 0) {
		fd = open("/dev/kmsg", O_WRONLY);
		if (fd < 0) {
			loge("Failed to open /dev/kmsg file\n");
			ret = -1;
		} else {
			(void)dup2(fd, 1);
			ret = execl("/bin/echo", "/bin/echo", msg, NULL);
			if (ret < 0) {
				loge("Failed on execl(/bin/echo)\n");
				ret = -1;
			}
			(void)close(fd);
		}
	}
	(void)wait(NULL);

	return ret;
}

/* coverity[HIS_metric_violation : FALSE] */
int klog_printl(const char *msg)
{
	int				ret		= 0;

	if (g_log_level == 1) {
		/* print */
		ret = print_to_kmsg_node(msg);
	}

	return ret;
}
