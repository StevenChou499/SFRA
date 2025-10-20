/*
 * fourier.h
 *
 *  Created on: Oct 15, 2025
 *      Author: Steven
 */

#ifndef FOURIER_H_
#define FOURIER_H_
#include <stdint.h>

#define PI (3.1415926535f)

typedef struct {
	float real;
	float imag;
} complex_t;

void print_complex(complex_t a);
complex_t complex_add(complex_t a, complex_t b);
complex_t complex_sub(complex_t a, complex_t b);
complex_t complex_mul(complex_t a, complex_t b);
complex_t complex_div(complex_t a, complex_t b);
complex_t complex_exp(float exp);
uint32_t get_bin_num(float samp_freq, uint32_t len, float target_freq);
complex_t dft_bin(complex_t *input_seq, uint32_t len, uint32_t freq_bin);
void dft(complex_t *input_seq, uint32_t len, complex_t *output_seq);
void idft(complex_t *input_seq, uint32_t len, complex_t *output_seq);

#endif /* FOURIER_H_ */
