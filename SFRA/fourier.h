/*
 * fourier.h
 *
 *  Created on: Oct 15, 2025
 *      Author: Steven
 */

#ifndef FOURIER_H_
#define FOURIER_H_

typedef struct {
	float real;
	float imag;
} complex_t;

void dft(complex_t *input_seq, uint32_t len, complex_t *output_seq);
void idft(complex_t *input_seq, uint32_t len, complex_t *output_seq);

#endif /* FOURIER_H_ */
