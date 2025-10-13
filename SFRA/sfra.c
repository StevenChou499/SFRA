#include "sfra.h"
#include <string.h>

sfra_t sfra;

uint8_t sfra_init(float sampling_rate_Hz, float freq_start, float freq_step)
{
	sfra.sampling_freq_Hz = sampling_rate_Hz;
	sfra.freq_start = freq_start;
	sfra.freq_step = freq_step;
	memset(sfra.mag_in, 0U, sizeof(float) * BODE_PLOT_POINTS);
	memset(sfra.pha_in, 0U, sizeof(float) * BODE_PLOT_POINTS);
	memset(sfra.mag_out, 0U, sizeof(float) * BODE_PLOT_POINTS);
	memset(sfra.pha_out, 0U, sizeof(float) * BODE_PLOT_POINTS);
	for (uint8_t i = 0U; i < BODE_PLOT_POINTS; i++) {
		sfra.freq_table[i] = freq_start * powf(freq_step, (float)i);
	}
}

void sfra_inject(void)
{

}

void sfra_collect(void)
{

}
