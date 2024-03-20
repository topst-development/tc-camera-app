// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Telechips Inc.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <limits.h>
#include "log.h"
#include "message_queue.h"
#include "basic_operation.h"
int message_queue_open(struct message_queue *msgq)
{
	int				fds[2]		= { -1, -1 };
	int				ret		= 0;

	if (msgq == NULL) {
		// msgq is NULL
		loge("msgq is NULL\n");
		ret = -1;
	} else {
		ret = pipe(fds);

		msgq->fd_read	= fds[0];
		msgq->fd_write	= fds[1];
	}

	return (int)ret;
}

int message_queue_close(const struct message_queue *msgq)
{
	int				ret		= 0;

	if (msgq == NULL) {
		loge("msgq is NULL\n");
		ret = -1;
	} else {
		ret = close(msgq->fd_read);
		if (ret < 0) {
			/* error */
			loge("close(fd_read), ret: %d\n", ret);
		}
		ret = close(msgq->fd_write);
		if (ret < 0) {
			/* error */
			loge("close(fd_write), ret: %d\n", ret);
		}
	}

	return (int)ret;
}

int message_queue_get(const struct message_queue *msgq,
	struct message *msg)
{
	char				*p		= (char *)msg;
	size_t				read_bytes	= 0;
	int				ret		= 0;

	if (msgq == NULL) {
		loge("msgq is NULL\n");
		ret = -1;
	} else {
		while (read_bytes < sizeof(*msg)) {
			/* coverity[misra_c_2012_rule_18_4_violation : FALSE] */
			ssize_t bytes = read(msgq->fd_read, p + read_bytes, sizeof(*msg) - read_bytes);

			if (bytes < 0) {
				loge("read_size(%lu - %lu)\n", sizeof(*msg), read_bytes);
				ret = -1;
			} else {
				/* increase */
				read_bytes += (size_t)bytes;
			}
		}
	}

	return ret;
}

int message_queue_put(const struct message_queue *msgq, const struct message *msg)
{
	const char			*p		= (const char *)msg;
	size_t				write_bytes	= 0;
	int				ret		= 0;

	if (msgq == NULL) {
		loge("msgq is NULL\n");
		ret = -1;
	} else {
		while (write_bytes < sizeof(*msg)) {
			/* coverity[misra_c_2012_rule_18_4_violation : FALSE] */
			ssize_t bytes = write(msgq->fd_write, p + write_bytes, sizeof(*msg) - write_bytes);

			if (bytes < 0) {
				loge("write_size(%lu - %lu)\n", sizeof(*msg), write_bytes);
				ret = -1;
			} else {
				/* increase */
				write_bytes += (size_t)bytes;
			}
		}
	}

	return ret;
}

int message_queue_is_empty(const struct message_queue *msgq)
{
	struct pollfd			pfd		= { 0, };
	int				ret		= 0;

	if (msgq == NULL) {
		loge("msgq is NULL\n");
		ret = -1;
	} else {
		(void)memset((void *)&pfd, 0, sizeof(pfd));

		pfd.fd		= msgq->fd_read;
		pfd.events	= POLLIN;

		if (poll(&pfd, 1, 0) == -1) {
			loge("poll(): %s\n", strerror(errno));
		}

		ret = ((s16_to_u32(pfd.revents) & (unsigned int)POLLIN) == 0U) ? 1 : 0;
	}

	return ret;
}

int message_queue_init(struct messenger *msger)
{
	int				ret		= 0;

	ret = message_queue_open(&msger->cmd);
	if (ret < 0) {
		/* error */
		loge("message_queue_open(cmd), ret: %d\n", ret);
	}

	ret = message_queue_open(&msger->ack);
	if (ret < 0) {
		/* error */
		loge("message_queue_open(ack), ret: %d\n", ret);
	}

	return ret;
}

int message_queue_deinit(const struct messenger *msger)
{
	int				ret		= 0;

	ret = message_queue_close(&msger->cmd);
	if (ret < 0) {
		/* error */
		loge("message_queue_close(cmd), ret: %d\n", ret);
	}

	ret = message_queue_close(&msger->ack);
	if (ret < 0) {
		/* error */
		loge("message_queue_close(ack), ret: %d\n", ret);
	}

	return ret;
}
