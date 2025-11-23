#include "sfra.h"
#include "fourier.h"
#include <string.h>
#include <math.h>
// MCU specific includes
#include "stm32f4xx_hal.h"

float sine_table[3600];
sfra_t sfra;

uint8_t tx_buffer[3000];
uint8_t rx_buffer[10];
uint32_t tx_len;
uint8_t rx_len;
uint32_t tx_index;

const cmd_format_t cmd_table[] = {
	{ NO_CMD        , 0U, null_handler        },
	{ SFRA_RESET    , 1U, sfra_reset          },
	{ START_SWEEP   , 1U, sfra_start_sweep    },
	{ GET_STATUS    , 1U, sfra_get_status     },
	{ GET_BODE      , 1U, sfra_get_bode       },
	{ SET_START_FREQ, 5U, sfra_set_start_freq },
	{ SET_STEP_FREQ , 5U, sfra_set_step_freq  },
	{ SET_SAMP_FREQ , 5U, sfra_set_samp_freq  },
	{ SET_INPUT_AMP , 5U, sfra_set_input_amp  },
};

extern UART_HandleTypeDef huart3;

int8_t sfra_init(float samp_freq,
				 float start_freq,
				 float step_freq,
				 float inj_amp)
{
	for (uint32_t i = 0U; i < 3600U; i++) {
		sine_table[i] = sinf(i * 2.0f * PI / 3600.0f);
	}

	if (samp_freq < 0.0f)
		return -1;
	if (start_freq < 0.0f)
		return -1;
	if (step_freq < 1.0f)
		return -1;
	if (inj_amp < 0.0f)
		return -1;

	memset(&sfra, 0U, sizeof(sfra_t));

	sfra.sampling_freq_Hz = samp_freq;
	sfra.freq_start = start_freq;
	sfra.freq_step = step_freq;
	sfra.inject_amplitude = inj_amp;

	float freq_limit = sfra.sampling_freq_Hz / 2.0f;
	float freq_exp = 1.0f;
	uint32_t index = 0U;
	for (; index < MAX_POINTS; index++) {
		float samp_freq = sfra.freq_start * freq_exp;
		if (samp_freq >= freq_limit) {
			sfra.freq_points = index;
			return 0;
		}
		sfra.freq_table[index] = samp_freq;
		freq_exp *= sfra.freq_step;
	}
	sfra.freq_points = index;

	return 0;
}

uint32_t sfra_get_sample_count(float sampling_rate_Hz, float target_freq_Hz)
{
	float cycle_sample_pts = sampling_rate_Hz / target_freq_Hz;
	if (cycle_sample_pts <= 5.0f)
		return (uint32_t) (cycle_sample_pts * 20); // sample for 200 cycles
	if (cycle_sample_pts <= 20.0f)
		return (uint32_t) (cycle_sample_pts * 20); // sample for 50 cycles
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
	if (sfra.current_state == DELAY) {
		sfra.delayed_cycles++;
		return;
	}
	if (sfra.current_state != SWEEPING)
		return;
	if (sfra.output_count != (sfra.input_count - 1U))
		return;

	sfra.result[sfra.freq_index].real += *output * cosf(sfra.current_angle);
	sfra.result[sfra.freq_index].imag -= *output * sinf(sfra.current_angle);
	sfra.current_angle +=
			sfra.freq_table[sfra.freq_index] * 2.0f * PI / sfra.sampling_freq_Hz;
	if (sfra.current_angle > 2.0f * PI)
		sfra.current_angle -= 2.0f * PI;
	sfra.output_count++;
	if (sfra.output_count == sfra.total_count[sfra.freq_index])
		sfra.current_state = SWEEP_DONE;
}

void sfra_update(void)
{
	// update current state from new command
	if (sfra.rx_state == REPLY_CMD) {
		cmd_table[sfra.received_cmd].cmd_handler(rx_buffer + 3);
		HAL_UART_Receive_DMA(&huart3, rx_buffer, 3U);
		sfra.rx_state = WAIT_HEAD;
	}

	switch (sfra.current_state) {
		case SFRA_INIT:
			if (!sfra_init(sfra.sampling_freq_Hz,
						   sfra.freq_start,
						   sfra.freq_step,
						   sfra.inject_amplitude))
				sfra.current_state = SWEEP_INIT;
			else
				sfra.current_state = IDLE;
			break;

		case SWEEP_INIT:
			sfra.total_count[sfra.freq_index] =
					sfra_get_sample_count(sfra.sampling_freq_Hz, sfra.freq_table[sfra.freq_index]);
			sfra.input_count = 0U;
			sfra.output_count = 0U;
			sfra.result[sfra.freq_index].real = 0.0f;
			sfra.result[sfra.freq_index].imag = 0.0f;
			sfra.current_angle = 0.0f;
			sfra.current_state = SWEEPING;
			break;

		case SWEEP_DONE:
			sfra.pha_out[sfra.freq_index] =
					atan2f(sfra.result[sfra.freq_index].imag, sfra.result[sfra.freq_index].real) / PI * 180.0f + 90.0f;
			if (sfra.pha_out[sfra.freq_index] >= 180.0f)
				sfra.pha_out[sfra.freq_index] -= 360.0f;
			sfra.mag_out[sfra.freq_index] =
					20.0f * log10f(sqrtf(sfra.result[sfra.freq_index].real * sfra.result[sfra.freq_index].real + sfra.result[sfra.freq_index].imag * sfra.result[sfra.freq_index].imag) / sfra.total_count[sfra.freq_index] * 2.0f / sfra.inject_amplitude);
			if (sfra.freq_index < sfra.freq_points - 1) {
				sfra.freq_index++;
				sfra.current_state = DELAY;
			} else {
				sfra.current_state = SFRA_DONE;
			}
			break;

		case DELAY:
			if (sfra.delayed_cycles >= DELAY_CYCLES) {
				sfra.current_state = SWEEP_INIT;
				sfra.delayed_cycles = 0U;
			}
			break;

		case IDLE:
		case SWEEPING:
		case SFRA_DONE:
		default:
			break;
	}
}

void null_handler(uint8_t *payload)
{
	(void) payload;
	return;
}

void sfra_reset(uint8_t *payload)
{
	(void) payload;
	sfra.current_state = IDLE;
	sfra_ret_ack(SFRA_RESET);
	return;
}

void sfra_start_sweep(uint8_t *payload)
{
	(void) payload;
	if (sfra.current_state != IDLE)
	{
		sfra_ret_nack(START_SWEEP);
		return;
	}

	sfra.current_state = SFRA_INIT;
	sfra_ret_ack(START_SWEEP);
	return;
}

void sfra_get_status(uint8_t *payload)
{
	(void) payload;
	sfra_ret_ack(GET_STATUS);
	return;
}

void sfra_get_bode(uint8_t *payload)
{
	if (sfra.current_state != SFRA_DONE) {
		sfra_ret_nack(GET_BODE);
		return;
	}

	sfra_ret_ack(GET_BODE);
	return;
}

void sfra_set_start_freq(uint8_t *payload)
{
	if (sfra.current_state != IDLE) {
		sfra_ret_nack(SET_START_FREQ);
		return;
	}

	sfra.freq_start = *(float *)(payload);
	sfra_ret_ack(SET_START_FREQ);
	return;
}

void sfra_set_step_freq(uint8_t *payload)
{
	if (sfra.current_state != IDLE) {
		sfra_ret_nack(SET_STEP_FREQ);
		return;
	}

	sfra.freq_step = *(float *)(payload);
	sfra_ret_ack(SET_STEP_FREQ);
	return;
}

void sfra_set_samp_freq(uint8_t *payload)
{
	if (sfra.current_state != IDLE) {
		sfra_ret_nack(SET_SAMP_FREQ);
		return;
	}

	sfra.sampling_freq_Hz = *(float *)(payload);
	sfra_ret_ack(SET_SAMP_FREQ);
	return;
}

void sfra_set_input_amp(uint8_t *payload)
{
	if (sfra.current_state != IDLE) {
		sfra_ret_nack(SET_INPUT_AMP);
		return;
	}

	sfra.inject_amplitude = *(float *)(payload);
	sfra_ret_ack(SET_INPUT_AMP);
	return;
}

void sfra_ret_ack(sfra_cmd_t command)
{
	uint32_t points = 0U;
	uint32_t index = 0U;
	tx_buffer[0] = 0xBB;
	tx_buffer[1] = command;
	switch (command)
	{
		case SFRA_RESET:
		case START_SWEEP:
		case SET_START_FREQ:
		case SET_STEP_FREQ:
		case SET_SAMP_FREQ:
		case SET_INPUT_AMP:
			*(uint16_t *)(tx_buffer + 2) = 2U;
			tx_buffer[4] = ACK;
			tx_buffer[5] = 0U;
			for (int i = 0; i < 5; i++) {
				tx_buffer[5] ^= tx_buffer[i];
			}
			break;
		case GET_STATUS:
			*(uint16_t *)(tx_buffer + 2) = 2U;
			tx_buffer[4] = sfra.current_state;
			tx_buffer[5] = 0U;
			for (int i = 0; i < 5; i++) {
				tx_buffer[5] ^= tx_buffer[i];
			}
			break;
		case GET_BODE:
			points = sfra.freq_points;
			*(uint16_t *)(tx_buffer + 2) = 3U * 4U * points + 1;

			index = 4U;
			memcpy(tx_buffer + index, (uint8_t *)sfra.freq_table, 4U * points);
			index += 4U * points;
			memcpy(tx_buffer + index, (uint8_t *)sfra.mag_out   , 4U * points);
			index += 4U * points;
			memcpy(tx_buffer + index, (uint8_t *)sfra.pha_out   , 4U * points);
			index += 4U * points;

			tx_buffer[index] = 0U;
			for (int i = 0; i < index; i++) {
				tx_buffer[index] ^= tx_buffer[i];
			}
			break;
		default:

	}
	tx_len = *(uint16_t *)(tx_buffer + 2) + 4U;
	if (tx_len > 512U) {
		sfra.tx_state = MULTI_PACKET;
		HAL_UART_Transmit_DMA(&huart3, tx_buffer, 512U);
		tx_len -= 512U;
		tx_index += 512U;
	} else {
		sfra.tx_state = SINGLE_PACKET;
		HAL_UART_Transmit_DMA(&huart3, tx_buffer, tx_len);
		tx_index = 0U;
	}
	return;
}

void sfra_ret_nack(sfra_cmd_t command)
{
	ack_nack_packet_t ret = {
		.header   = 0xBB,
		.cmd      = command,
		.len      = 1U,
		.nack     = NACK,
		.checksum = 0U,
	};

	for (int i = 0; i < 5; i++) {
		ret.checksum ^= *((uint8_t *)(&ret) + i);
	}

	HAL_UART_Transmit(&huart3, (uint8_t *)&ret, sizeof(ret), HAL_MAX_DELAY);
	return;
}

/*
 * MCU specific serial functions
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	switch (sfra.tx_state)
	{
		case MULTI_PACKET:
			if (tx_len > 512U) {
				HAL_UART_Transmit_DMA(&huart3, tx_buffer + tx_index, 512U);
				tx_len -= 512U;
				tx_index += 512U;
			} else {
				HAL_UART_Transmit_DMA(&huart3, tx_buffer + tx_index, tx_len);
				tx_len =0U;
				tx_index = 0U;
				sfra.tx_state = NO_PACKET;
			}
			break;

		case NO_PACKET:
		case SINGLE_PACKET:
		default:
			sfra.tx_state = NO_PACKET;
			break;
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (WAIT_HEAD == sfra.rx_state) {
		if (rx_buffer[0] != 0xAA)
			return;
		if (rx_buffer[1] == NO_CMD || rx_buffer[1] > SET_INPUT_AMP)
			return;

		uint16_t payload_len = cmd_table[rx_buffer[2]].payload_len;
		HAL_UART_Receive_DMA(&huart3, rx_buffer + 3, payload_len);
		sfra.rx_state = WAIT_PAYLOAD;
	} else { // WAIT_PAYLOAD == sfra.rx_state
		sfra.received_cmd = rx_buffer[1];
		sfra.rx_state = REPLY_CMD;
	}
}
