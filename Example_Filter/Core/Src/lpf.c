/*
 * lpf.c
 *
 *  Created on: Oct 13, 2025
 *      Author: Steven
 */
#include "lpf.h"

static lpf_t ac_lpf;

lpf_t *ac_lpf_p = &ac_lpf;

void lpf_init(lpf_t *lpf, float cut_off_freq_Hz, float samp_rate)
{
	lpf->samp_rate = samp_rate;
	lpf->time_period = 1.0f / lpf->samp_rate;
	lpf->w_c = 2.0f * PI * cut_off_freq_Hz;
	lpf->b0 = (lpf->w_c * lpf->time_period) / (2.0f + lpf->w_c * lpf->time_period);
	lpf->b1 = (lpf->w_c * lpf->time_period) / (2.0f + lpf->w_c * lpf->time_period);
	lpf->a1 = (lpf->w_c * lpf->time_period - 2.0f) / (lpf->w_c * lpf->time_period + 2.0f);

	lpf->last_input = 0.0f;
	lpf->last_output = 0.0f;
}

void lpf_update(lpf_t *lpf, float new_input)
{
	lpf->input = new_input;
	lpf->output = lpf->b0 * lpf->input +
			      lpf->b1 * lpf->last_input -
				  lpf->a1 * lpf->last_output;

	lpf->last_input = lpf->input;
	lpf->last_output = lpf->output;
}

