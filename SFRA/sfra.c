#include "sfra.h"
#include "fourier.h"
#include <string.h>
#include <math.h>
// MCU specific includes
#include "stm32f4xx_hal.h"

float sine_table[3600];
sfra_t sfra;

uint8_t tx_buffer[200];
uint8_t rx_buffer[10];
uint32_t tx_len;
uint8_t rx_len;

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
	uint32_t index = 0U;
	for (; index < MAX_POINTS; index++) {
		float samp_freq = freq_start * freq_exp;
		if (samp_freq >= freq_limit) {
			sfra.freq_points = index;
			sfra.current_state = IDLE;
			return -1;
		}
		sfra.freq_table[index] = samp_freq;
		freq_exp *= freq_step;
	}
	sfra.freq_points = index;
	sfra.current_state = IDLE;

	return 0;
}

uint32_t sfra_get_sample_count(float sampling_rate_Hz, float target_freq_Hz)
{
	float cycle_sample_pts = sampling_rate_Hz / target_freq_Hz;
	if (cycle_sample_pts <= 20.0f)
		return (uint32_t) (cycle_sample_pts * 200); // sample for 200 cycles
	else
		return (uint32_t) (cycle_sample_pts * 20);  // sample for 20 cycles
}

float sfra_inject(float input)
{
	if (sfra.current_state != SWEEPING)
		return input;

	input += sfra.inject_amplitude * sinf(sfra.current_angle);
	sfra.input_count++;
	return input;
}

void sfra_collect(float *output)
{
	if (sfra.current_state != SWEEPING)
		return;
	if (sfra.output_count != (sfra.input_count - 1U))
		return;

	sfra.real_part += *output * cosf(sfra.current_angle);
	sfra.imag_part -= *output * sinf(sfra.current_angle);
	sfra.current_angle +=
			sfra.freq_table[sfra.current_freq_index] * 2.0f * PI / sfra.sampling_freq_Hz;
	if (sfra.current_angle > 2.0f * PI)
		sfra.current_angle -= 2.0f * PI;
	sfra.output_count++;
	if (sfra.output_count == sfra.total_count)
		sfra.current_state = SWEEP_DONE;
}

void sfra_update(void)
{
	// update current state from new command
	if (sfra.received_cmd)
		cmd_table[sfra.received_cmd].cmd_handler(rx_buffer + 2);

	if (sfra.current_state == SWEEP_DONE) {
	  	sfra.pha_out[sfra.current_freq_index] =
	  			atan2f(sfra.imag_part, sfra.real_part) / PI * 180.0f + 90.0f;
	  	sfra.mag_out[sfra.current_freq_index] =
	  			20.0f * log10f(sqrtf(sfra.real_part * sfra.real_part + sfra.imag_part * sfra.imag_part) / sfra.total_count * 2.0f / sfra.inject_amplitude);
	  	if (sfra.current_freq_index < sfra.freq_points - 1) {
	  		sfra.current_freq_index++;
	  		sfra.total_count = sfra_get_sample_count(100e3f, sfra.freq_table[sfra.current_freq_index]);
	  		sfra.input_count = 0U;
	  		sfra.output_count = 0U;
	  		sfra.real_part = 0.0f;
	  		sfra.imag_part = 0.0f;
	  		sfra.current_angle = 0.0f;
	  		sfra.current_state = SWEEPING;
	    } else {
	  	  sfra.current_state = SFRA_DONE;
	    }
	}
}

void sfra_reset(uint8_t *payload)
{
	return;
}

void sfra_start_sweep(uint8_t *payload)
{
	return;
}

void sfra_get_status(uint8_t *payload)
{
	return;
}

void sfra_get_bode(uint8_t *payload)
{
	return;
}

void sfra_set_start_freq(uint8_t *payload)
{
	return;
}

void sfra_set_step_freq(uint8_t *payload)
{
	return;
}

void sfra_set_samp_freq(uint8_t *payload)
{
	return;
}

/*
 * MCU specific serial functions
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (WAIT_HEAD == sfra.rx_state) {
		if (rx_buffer[0] != 0xAA)
			return;
		if (rx_buffer[1] == 0U)
			return;

		uint16_t payload_len = cmd_table[rx_buffer[1]].payload_len;
		HAL_UART_Receive_DMA(&huart3, rx_buffer + 2, payload_len);
		sfra.rx_state = WAIT_PAYLOAD;
	} else { // WAIT_PAYLOAD == sfra.rx_state
		sfra.received_cmd = rx_buffer[1];
		sfra.rx_state = REPLY_CMD;
	}
}
