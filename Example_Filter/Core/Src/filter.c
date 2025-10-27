/*
 * lpf.c
 *
 *  Created on: Oct 13, 2025
 *      Author: Steven
 */
#include <filter.h>

static lpf_t ac_lpf;
static pi_t  ac_pi;
static pd_t  ac_pd;

lpf_t *ac_lpf_p = &ac_lpf;
pi_t  *ac_pi_p = &ac_pi;
pd_t  *ac_pd_p = &ac_pd;

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

void lpf_update(lpf_t *lpf, float new_input, float *output)
{
	lpf->input = new_input;
	lpf->output = lpf->b0 * lpf->input +
			      lpf->b1 * lpf->last_input -
				  lpf->a1 * lpf->last_output;
	*output = lpf->output;

	lpf->last_input = lpf->input;
	lpf->last_output = lpf->output;
}

void pi_init(pi_t *pi, float Kp, float Ki, float samp_rate)
{
	pi->samp_rate = samp_rate;
	pi->time_period = 1.0f / pi->samp_rate;
	pi->Kp = Kp;
	pi->Ki = Ki;
	pi->b0 = pi->Kp + (pi->Ki * pi->Kp * pi->time_period / 2.0f);
	pi->b1 = (pi->Ki * pi->Kp * pi->time_period / 2.0f) - pi->Kp;
	pi->a1 = -1.0f;

	pi->last_input = 0.0f;
	pi->last_output = 0.0f;
}

void pi_update(pi_t *pi, float new_input, float *output)
{
	pi->input = new_input;
	pi->output = pi->b0 * pi->input +
			     pi->b1 * pi->last_input -
				 pi->a1 * pi->last_output;
	*output = pi->output;

	pi->last_input = pi->input;
	pi->last_output = pi->output;
}

void pd_init(pd_t *pd, float Kp, float Kd, float samp_rate)
{
	pd->samp_rate = samp_rate;
	pd->time_period = 1.0f / pd->samp_rate;
	pd->Kp = Kp;
	pd->Kd = Kd;
	pd->b0 = pd->Kp + (2.0f * pd->Kp * pd->Kd / pd->time_period);
	pd->b1 = pd->Kp - (2.0f * pd->Kp * pd->Kd / pd->time_period);
	pd->a1 = 1.0f;

	pd->last_input = 0.0f;
	pd->last_output = 0.0f;
}

void pd_update(pd_t *pd, float new_input, float *output)
{
	pd->input = new_input;
	pd->output = pd->b0 * pd->input +
				 pd->b1 * pd->last_input -
				 pd->a1 * pd->last_output;
	*output = pd->output;

	pd->last_input = pd->input;
	pd->last_output = pd->output;
}

