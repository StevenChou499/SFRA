/*
 * lpf.h
 *
 *  Created on: Oct 13, 2025
 *      Author: Steven
 */

#ifndef INC_FILTER_H_
#define INC_FILTER_H_

//#define PI (3.141592653589793f)

/*
 * Low pass filter :
 *
 *  Y(s)       a
 * ------ = -------
 *  U(s)     s + a
 */
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

/*
 * PI controller :
 *
 *  Y(s)               Ki
 * ------ = Kp * (1 + ----)
 *  U(s)                s
 */
typedef struct {
	float samp_rate;   // PI's sampling rate
	float time_period; // 1 / samp_rate
	float Kp;          // Gain coefficient
	float Ki;          // Integral coefficient

	//  Y(z)     b0 + b1 * z^-1
	// ------ = ----------------
	//  U(z)     1 + a1 * z^-1
	float b0;
	float b1;
	float a1;

	float input;
	float last_input;
	float output;
	float last_output;
} pi_t;

/*
 * PD controller :
 *
 *  Y(s)
 * ------ = Kp * (1 + Kds)
 *  U(s)
 */
typedef struct {
	float samp_rate;   // PD's sampling rate
	float time_period; // 1 / samp_rate
	float Kp;          // Gain coefficient
	float Kd;          // Derivative coefficient

	//  Y(z)     b0 + b1 * z^-1
	// ------ = ----------------
	//  U(z)     1 + a1 * z^-1
	float b0;
	float b1;
	float a1;

	float input;
	float last_input;
	float output;
	float last_output;
} pd_t;

extern lpf_t *ac_lpf_p;
extern pi_t  *ac_pi_p;
extern pd_t  *ac_pd_p;

void lpf_init(lpf_t *lpf, float cut_off_freq_Hz, float samp_rate);
void lpf_update(lpf_t *lpf, float new_input, float *output);
void pi_init(pi_t *pi, float Kp, float Ki, float samp_rate);
void pi_update(pi_t *pi, float new_input, float *output);
void pd_init(pd_t *pd, float Kp, float Kd, float samp_rate);
void pd_update(pd_t *pd, float new_input, float *output);
#endif /* INC_FILTER_H_ */
