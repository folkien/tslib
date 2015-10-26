/*
 *  tslib/plugins/pressure.c
 *
 *  Copyright (C) 2003 Texas Instruments, Inc.
 *
 *  Based on:
 *    tslib/plugins/linear.c
 *    Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * Ensure a release is always pressure 0, and that a press is always >=1.
 */
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "config.h"
#include "tslib.h"
#include "tslib-filter.h"

struct tslib_pressure {
	struct tslib_module_info module;
	unsigned int	threshold;
};

// If sample pressure is lower than threshold we change this data to "release data".
static int
pressure_read(struct tslib_module_info *info, struct ts_sample *input_samples, int nr)
{
	struct tslib_pressure *p = (struct tslib_pressure *)info;
	int samples_count;

	samples_count = info->next->ops->read(info->next, input_samples, nr);
	if (samples_count >= 0) {
		int i;
		struct ts_sample *current_sample;

		for (current_sample = input_samples, i = 0; i < samples_count; i++, current_sample++) {
            // for pressure lower than threshold we don't accept data and set it as release.
			if ((current_sample->pressure != 0) && (current_sample->pressure < p->threshold)) {
                current_sample->pressure = 0;
            }
            // for pressure higher than threshold we accept the data
		}
	}
	return samples_count;
}

static int pressure_fini(struct tslib_module_info *info)
{
	free(info);
	return 0;
}

static const struct tslib_ops pressure_ops =
{
	.read	= pressure_read,
	.fini	= pressure_fini,
};

static int threshold_vars(struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_pressure *p = (struct tslib_pressure *)inf;
	unsigned long v;
	int err = errno;

	v = strtoul(str, NULL, 0);

	if (v == ULONG_MAX && errno == ERANGE)
		return -1;

	errno = err;
	switch ((int)data) {
	case 0:
		p->threshold = v;
		break;

	default:
		return -1;
	}
	return 0;
}


static const struct tslib_vars pressure_vars[] =
{
	{ "threshold",	(void *)0, threshold_vars }
};

#define NR_VARS (sizeof(pressure_vars) / sizeof(pressure_vars[0]))

TSAPI struct tslib_module_info *pressure_mod_init(struct tsdev *dev, const char *params)
{

	struct tslib_pressure *p;

	p = malloc(sizeof(struct tslib_pressure));
	if (p == NULL)
		return NULL;

	p->module.ops = &pressure_ops;

	p->threshold = 150;

	/*
	 * Parse the parameters.
	 */
	if (tslib_parse_vars(&p->module, pressure_vars, NR_VARS, params)) {
		free(p);
		return NULL;
	}

	return &p->module;
}

#ifndef TSLIB_STATIC_PRESSURE_MODULE
	TSLIB_MODULE_INIT(pressure_mod_init);
#endif
