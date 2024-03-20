/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) Telechips Inc.
 */

#ifndef REVERSE_SWITCH_H
#define REVERSE_SWITCH_H

#include <stdint.h>

struct switch_t {
	int		id;
	int		fd;
};

extern int switch_open_device(struct switch_t *dev);
extern int switch_close_device(const struct switch_t *dev);
extern int switch_get_state(const struct switch_t *dev);

#endif//REVERSE_SWITCH_H
