/*
 *  tslib/plugins/iir.c
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
 * Ensure a release is always iir 0, and that a press is always >=1.
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

struct tslib_iir {
	struct tslib_module_info module;
	unsigned int A0;
	unsigned int B0;
	unsigned int B1;
};


static int
iir_read(struct tslib_module_info *info, struct ts_sample *input_samples, int nr)
{
	struct tslib_iir *p = (struct tslib_iir *)info;
	int readed_samples;
    // Filter y memory
    static struct ts_sample fy[2];
    // Filter information about touchpanel pressing
    static int ts_pressed = 0;

    // Reading samples from bottom layer
	readed_samples = info->next->ops->read(info->next, input_samples, nr);
    // If there are samples
	if (readed_samples >= 0) {
		int i;
		struct ts_sample *current_sample;

		for (current_sample = input_samples, i = 0 ; i < readed_samples; i++, current_sample++) {
            if (ts_pressed == 0) {
                // Taking first sample as reference
                fy[1].x = fy[0].x = current_sample->x;
                fy[1].y = fy[0].y = current_sample->y;
#ifdef DEBUG
                printf("Ref. : %d %d %d\n",fy[0].x, fy[0].y, fy[0].iir);
#endif
            }
            else {
#ifdef DEBUG
                printf("fy[1] : %d %d %d.\n", fy[1].x, fy[1].y, fy[1].iir);
                printf("fy[0] : %d %d %d.\n", fy[0].x, fy[0].y, fy[0].iir);
                printf("Sample : %d %d %d ->", current_sample->x, current_sample->y, current_sample->pressure);
#endif

                // Calculating the IIR filter equation. We always divide by 8.
                current_sample->x = (p->B1*fy[1].x + p->B0*fy[0].x + p->A0*current_sample->x)>>3;
                current_sample->y = (p->B1*fy[1].y + p->B0*fy[0].y + p->A0*current_sample->y)>>3;
#ifdef DEBUG
                printf(" %d %d %d \n", current_sample->x, current_sample->y, current_sample->pressure);
#endif

                //Shifting data in filter memory
                fy[1].x = fy[0].x;
                fy[1].y = fy[0].y;
                fy[0].x = current_sample->x;
                fy[0].y = current_sample->y;
            }
            ts_pressed = current_sample->pressure;
		}
	}
	return readed_samples;
}

static int iir_fini(struct tslib_module_info *info)
{
	free(info);
	return 0;
}

static const struct tslib_ops iir_ops =
{
	.read	= iir_read,
	.fini	= iir_fini,
};

static int threshold_vars(struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_iir *p = (struct tslib_iir *)inf;
	unsigned long v;
	int err = errno;

	v = strtoul(str, NULL, 0);

	if (v == ULONG_MAX && errno == ERANGE)
		return -1;

	errno = err;
	switch ((int)data) {
	case 0:
		p->A0 = v;
		break;

	case 1:
		p->B0 = v;
		break;

	case 2:
		p->B1 = v;
		break;


	default:
		return -1;
	}
	return 0;
}


static const struct tslib_vars iir_vars[] =
{
	{ "A0",	(void *)0, threshold_vars },
	{ "B0",	(void *)1, threshold_vars },
	{ "B1",	(void *)2, threshold_vars }
};

#define NR_VARS (sizeof(iir_vars) / sizeof(iir_vars[0]))

TSAPI struct tslib_module_info *iir_mod_init(struct tsdev *dev, const char *params)
{

	struct tslib_iir *p;

	p = malloc(sizeof(struct tslib_iir));
	if (p == NULL)
		return NULL;

	p->module.ops = &iir_ops;
	p->A0 = 1;
    p->B0 = 3;
    p->B1 = 4;
#ifdef DEBUG
    printf("Threshold set to default : %d",p->threshold);
#endif

	/*
	 * Parse the parameters.
	 */
	if (tslib_parse_vars(&p->module, iir_vars, NR_VARS, params)) {
		free(p);
		return NULL;
	}

	return &p->module;
}

#ifndef TSLIB_STATIC_iir_MODULE
	TSLIB_MODULE_INIT(iir_mod_init);
#endif
