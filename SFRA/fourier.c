/*
 * fourier.c
 *
 *  Created on: Oct 15, 2025
 *      Author: Steven
 */
#include "fourier.h"
#include <math.h>

void dft(complex_t *input_seq, uint32_t len, complex_t *output_seq)
{
	if (!output_seq)
		return;

	for (int k = 0; k < len; k++) {
		for (int i = 0; i < len; i++) {

		}
	}
}

void idft(complex_t *input_seq, uint32_t len, complex_t *output_seq)
{

}
