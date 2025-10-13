#include <stdint.h>

#define BODE_PLOT_POINTS (10U)

typedef struct {
	float mag_in[BODE_PLOT_POINTS];
	float pha_in[BODE_PLOT_POINTS];
	float mag_out[BODE_PLOT_POINTS];
	float pha_out[BODE_PLOT_POINTS];
	float freq_start;
	float freq_step;
	float freq_table[BODE_PLOT_POINTS];
	float sampling_freq_Hz;
} sfra_t;

uint8_t sfra_init(float sampling_rate_Hz, float freq_start, float freq_step);
void sfra_inject(void);
void sfra_collect(void);
