/*
 * lpf.h
 *
 *  Created on: Oct 13, 2025
 *      Author: Steven
 */

#ifndef INC_LPF_H_
#define INC_LPF_H_

#define PI (3.141592653589793f)

typedef struct {
	float  samp_rate;   // lpf's sampling rate
	float  time_period; // 1 / samp_rate
	float  w_c;         // BW coefficient

	//  Y(z)     b0 + b1 * z^-1
	// ------ = ----------------
	//  U(z)     1 + a1 * z^-1
	float  b0;
	float  b1;
	float  a1;

	float  input;
	float  last_input;
	float  output;
	float  last_output;
} lpf_t;

extern lpf_t *ac_lpf_p;

void lpf_init(lpf_t *lpf, float cut_off_freq_Hz, float samp_rate);
void lpf_update(lpf_t *lpf, float new_input, float *output);

#endif /* INC_LPF_H_ */
