#include <stdint.h>

#define MAX_POINTS (100U)

typedef enum {
	IDLE,
	GEN_TABLE,
	SWEEPING,
	SWEEP_DONE,
} sfra_state_t;

typedef struct sfra {
	float mag_in[MAX_POINTS];
	float pha_in[MAX_POINTS];
	float mag_out[MAX_POINTS];
	float pha_out[MAX_POINTS];
	float real_part;
	float imag_part;
	float freq_start;
	float freq_step;
	uint32_t freq_points;
	float freq_table[MAX_POINTS];
	uint32_t current_freq_index;
	float current_angle;
	float inject_amplitude;
	uint32_t input_count;
	uint32_t output_count;
	uint32_t total_count;
	float sampling_freq_Hz;
	sfra_state_t current_state;
} sfra_t;

extern sfra_t sfra_st;

uint8_t sfra_init(float sampling_rate_Hz,
	              float freq_start,
				  float freq_step,
				  float input_amplitude);
uint32_t sfra_get_sample_count(float sampling_rate_Hz, float target_freq_Hz);
void sfra_inject(float *input);
void sfra_collect(float *output);
