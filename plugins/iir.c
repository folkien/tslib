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
//	unsigned int	pmax;
};

static struct ts_sample fy[2];
static int ts_pressed = 0;

#define ABS(X) ( (X>0) ? X : -X  )

// function return number of samples that are in read buffer. This hungry function could
// consume data and return less samples than received.
static int
pressure_read(struct tslib_module_info *info, struct ts_sample *input_samples, int nr)
{
	struct tslib_pressure *p = (struct tslib_pressure *)info;
	int readed_samples;
    //static struct ts_sample *last_sample;

    // Reading samples from bottom layer
	readed_samples = info->next->ops->read(info->next, input_samples, nr);
    // If there are samples
	if (readed_samples >= 0) {
		int i, distance;
		struct ts_sample *current_sample;

		for (current_sample = input_samples, i = 0 ; i < readed_samples; i++, current_sample++) {
            if (ts_pressed == 0) {
                // Taking first sample as reference
                fy[1].x = fy[0].x = current_sample->x;
                fy[1].y = fy[0].y = current_sample->y;
                printf("Ref. : %d %d %d\n",fy[0].x, fy[0].y, fy[0].pressure);
            }
            else {
                printf("fy[1] : %d %d %d.\n", fy[1].x, fy[1].y, fy[1].pressure);
                printf("fy[0] : %d %d %d.\n", fy[0].x, fy[0].y, fy[0].pressure);
                printf("Sample : %d %d %d ->", current_sample->x, current_sample->y, current_sample->pressure);

                // Calculating the average
                current_sample->x = (4*fy[1].x + 3*fy[0].x + current_sample->x)>>3;
                current_sample->y = (4*fy[1].y + 3*fy[0].y + current_sample->y)>>3;

                printf(" %d %d %d \n", current_sample->x, current_sample->y, current_sample->pressure);

                //Shifting
                fy[1].x = fy[0].x;
                fy[1].y = fy[0].y;
                fy[0].x = current_sample->x;
                fy[0].y = current_sample->y;
            }

            //Calculating of distance and getting pressed value
            //distance = (avg_sample.x - current_sample->x) + (avg_sample.y-current_sample->y);
            //if (distance < 0) distance *= -1;

            ts_pressed = current_sample->pressure;

/*            printf("Distance check d=%d > thre=%d.\n",distance,p->threshold);
            // We drop because of distance
            if (distance > p->threshold) {
                //current_sample->pressure = 0;*/
                //printf("Sample %d, %d, %d ",current_sample->x, current_sample->y, current_sample->pressure);
                //current_sample->x = avg_sample.x;
                //current_sample->y = avg_sample.y;
               // printf("changed to  %d %d %d.\n",current_sample->x, current_sample->y, current_sample->pressure);
           // }

            // for pressure lower than threshold we don't accept data i drop it.
            /*printf("Checking %d < %d.\n", current_sample->pressure,p->threshold);
			if ((current_sample->pressure != 0) && (current_sample->pressure < p->threshold)) {
				int left = readed_samples - counter - 1;
                current_sample->pressure = 0;
                printf("Sample %d, %d, %d changed to release.\n",current_sample->x, current_sample->y, current_sample->pressure);
            }*/

            // for pressure higher than threshold we accept the data
		}
	}
	return readed_samples;
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
        printf("Threshold set to  : %d",p->threshold);
		break;

/*	case 1:
		p->pmax = v;
		break;
*/

	default:
		return -1;
	}
	return 0;
}


static const struct tslib_vars pressure_vars[] =
{
	{ "threshold",	(void *)0, threshold_vars }
//	{ "pmax",	(void *)1, threshold_vars }
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
    printf("Threshold set to default : %d",p->threshold);
//	p->pmax = INT_MAX;

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
