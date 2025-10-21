#include "sfra.h"
#include "fourier.h"
#include <string.h>
#include <math.h>

float sine_table[3600];
sfra_t sfra;

uint8_t sfra_init(float sampling_rate_Hz,
	              float freq_start,
				  float freq_step,
				  float input_amplitude)
{
	for (uint32_t i = 0U; i < 3600U; i++) {
		sine_table[i] = sinf(i * 2.0f * PI / 3600.0f);
	}

	memset(&sfra, 0U, sizeof(sfra_t));
	sfra.sampling_freq_Hz = sampling_rate_Hz;
	sfra.freq_start = freq_start;
	sfra.freq_step = freq_step;
	sfra.inject_amplitude = input_amplitude;

	float freq_limit = sampling_rate_Hz / 2.0f;
	float freq_exp = 1.0f;
	for (uint8_t i = 0U; i < MAX_POINTS; i++) {
		float samp_freq = freq_start * freq_exp;
		if (samp_freq >= freq_limit)
			return -1;
		sfra.freq_table[i] = samp_freq;
		freq_exp *= freq_step;
	}

	sfra.current_state = IDLE;

	return 0;
}

uint32_t sfra_get_sample_count(float sampling_rate_Hz, float target_freq_Hz)
{
	float cycle_sample_pts = sampling_rate_Hz / target_freq_Hz;
	if (cycle_sample_pts <= 20.0f)
		return (uint32_t) (cycle_sample_pts * 100); // sample for 100 cycles
	else
		return (uint32_t) (cycle_sample_pts * 10);  // sample for 10 cycles
}

float sfra_inject(float input)
{
	if (sfra.current_state != SWEEPING)
		return -1.0f;

	input += sfra.inject_amplitude * sinf(sfra.current_angle);
	sfra.input_count++;
	return input;
}

void sfra_collect(float *output)
{
	if (sfra.current_state != SWEEPING)
		return;
	if (sfra.output_count != (sfra.input_count - 1))
		return;

	sfra.real_part += *output * cosf(sfra.current_angle);
	sfra.imag_part -= *output * sinf(sfra.current_angle);
	sfra.current_angle +=
			sfra.freq_table[sfra.current_freq_index] * 2.0f * PI / sfra.sampling_freq_Hz;
	sfra.output_count++;
}
