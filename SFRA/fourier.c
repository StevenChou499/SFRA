/*
 * fourier.c
 *
 *  Created on: Oct 15, 2025
 *      Author: Steven
 */
#include "fourier.h"
#include <math.h>
#include <stdio.h>

void print_complex(complex_t a)
{
	printf("real: %f, imag: %f\n", a.real, a.imag);
}

complex_t complex_add(complex_t a, complex_t b)
{
	return (complex_t) { a.real + b.real, a.imag + b.imag };
}

complex_t complex_sub(complex_t a, complex_t b)
{
	return (complex_t) { a.real - b.real, a.imag - b.imag };
}

complex_t complex_mul(complex_t a, complex_t b)
{
	return (complex_t) { a.real * b.real - a.imag * b.imag,
	                     a.real * b.imag + a.imag * b.real };
}

complex_t complex_div(complex_t a, complex_t b)
{
	float den = b.real * b.real + b.imag * b.imag;
	if (den == 0.0f)
		return (complex_t) { 0.0f, 0.0f };

	return (complex_t) {
		(a.real * b.real + a.imag * b.imag) / den,
	    (a.imag * b.real - a.real * b.imag) / den
	};
}

complex_t complex_exp(float exp)
{
	return (complex_t) { cosf(exp), sinf(exp) };
}

uint32_t get_bin_num(float samp_freq, uint32_t len, float target_freq)
{
	float base_freq = (float) samp_freq / (float) len;
	return target_freq / base_freq;
}

complex_t dft_bin(complex_t *input_seq, uint32_t len, uint32_t freq_bin)
{
	complex_t bin_result = { 0.0f, 0.0f };
	for (uint32_t n = 0; n < len; n++) {
		complex_t exp = complex_exp(-2 * PI * n * freq_bin / len);
		bin_result = complex_add(bin_result, complex_mul(input_seq[n], exp));
	}
	return bin_result;
}

void dft(complex_t *input_seq, uint32_t len, complex_t *output_seq)
{
	if (!input_seq || !output_seq)
		return;

	for (int k = 0; k < len; k++) {
		output_seq[k] = dft_bin(input_seq, len, k);
	}
	return;
}

complex_t idft_bin(complex_t *input_seq, uint32_t len, uint32_t freq_bin)
{
	complex_t bin_result = { 0.0f, 0.0f };
	for (int n = 0; n < len; n++) {
		complex_t exp = complex_exp(2 * PI * n * freq_bin / len);
		bin_result = complex_add(bin_result, complex_mul(input_seq[n], exp));
	}
	return (complex_t) { bin_result.real / len, bin_result.imag / len };
}

void idft(complex_t *input_seq, uint32_t len, complex_t *output_seq)
{
	if (!input_seq || !output_seq)
		return;

	for (int k = 0; k < len; k++) {
		output_seq[k] = idft_bin(input_seq, len, k);
	}
	return;
}
