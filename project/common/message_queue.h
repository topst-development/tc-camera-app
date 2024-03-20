/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) Telechips Inc.
 */

#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <stdint.h>

struct message {
	unsigned int		command;
	unsigned int		arg1;
	unsigned int		arg2;
	void			*arg3;
	void			*arg4;
};

struct message_queue {
	int			fd_read;
	int			fd_write;
};

struct messenger {
	struct message_queue	cmd;
	struct message_queue	ack;
};

extern int message_queue_open(struct message_queue *msgq);
extern int message_queue_close(const struct message_queue *msgq);
extern int message_queue_get(const struct message_queue *msgq, struct message *msg);
extern int message_queue_put(const struct message_queue *msgq, const struct message *msg);
extern int message_queue_is_empty(const struct message_queue *msgq);

extern int message_queue_init(struct messenger *msger);
extern int message_queue_deinit(const struct messenger *msger);

#endif//MESSAGE_QUEUE_H
