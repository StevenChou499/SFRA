#include <stdint.h>
#include "fourier.h"

#define IRQ_SAMP_FREQ   (100e3f)
#define MAX_POINTS      (100U)

typedef enum rx_state {
	WAIT_HEAD,
	WAIT_PAYLOAD,
	REPLY_CMD,
} rx_state_t;

typedef enum tx_state {
	NO_PACKET,
	SINGLE_PACKET,
	MULTI_PACKET,
} tx_state_t;

typedef enum sfra_cmd {
	NO_CMD,
	SFRA_RESET,
	START_SWEEP,
	GET_STATUS,
	GET_BODE,
	SET_START_FREQ,
	SET_STEP_FREQ,
	SET_SAMP_FREQ,
	SET_INPUT_AMP,
} sfra_cmd_t;

typedef enum ack_cmd {
	ACK,
	NACK,
} ack_cmd_t;

typedef struct ack_nack_packet {
	uint8_t  header;
	uint8_t  cmd;
	uint16_t len;
	uint8_t  nack;
	uint8_t  checksum;
} ack_nack_packet_t;

typedef struct cmd_format {
	sfra_cmd_t cmd_id;              // Command ID
	uint8_t payload_len;            // Length of pay-load
	void (*cmd_handler)(uint8_t *payload);
} cmd_format_t;

typedef enum sfra_state {
	IDLE,
	SFRA_INIT,
	SWEEP_INIT,
	SWEEPING,
	SWEEP_DONE,
	SFRA_DONE,
} sfra_state_t;

typedef struct sfra_st {
	float mag_in[MAX_POINTS];         // Input magnitude
	float pha_in[MAX_POINTS];         // Input phase
	float mag_out[MAX_POINTS];        // Output magnitude
	float pha_out[MAX_POINTS];        // Output phase
	complex_t result[MAX_POINTS];     // complex for calculating SFRA
	float freq_start;                 // Starting sweeping frequency
	float freq_step;                  // Frequency between every sweep
	uint32_t freq_points;             // Total sweeping frequency points
	float freq_table[MAX_POINTS];     // All the sweeping frequencies in table
	uint32_t freq_index;              // Current sweeping frequency index
	float current_angle;              // SFRA current sweeping angle
	float inject_amplitude;           // Input amplitude for every sweep
	uint32_t input_count;             // Number of points injected in a single sweep
	uint32_t output_count;            // Number of points collected in a single sweep
	uint32_t total_count[MAX_POINTS]; // Number of points total needed in a single sweep
	float sampling_freq_Hz;           // SFRA's sampling frequency
	tx_state_t tx_state;              // SFRA's TX command state
	rx_state_t rx_state;              // SFRA's RX command state
	sfra_cmd_t received_cmd;          // SFRA's received command
	sfra_state_t current_state;       // SFRA's current state
} sfra_t;

extern sfra_t sfra;
extern uint8_t tx_buffer[1500];
extern uint8_t rx_buffer[10];
extern uint32_t tx_len;
extern uint8_t rx_len;

int8_t sfra_init(float samp_freq,
				 float start_freq,
				 float step_freq,
				 float inj_amp);
uint32_t sfra_get_sample_count(float sampling_rate_Hz, float target_freq_Hz);
float sfra_inject(float input);
void sfra_collect(float *output);
void sfra_update(void);

/* Command related handler functions */
void null_handler(uint8_t *payload);
void sfra_reset(uint8_t *payload);
void sfra_start_sweep(uint8_t *payload);
void sfra_get_status(uint8_t *payload);
void sfra_get_bode(uint8_t *payload);
void sfra_set_start_freq(uint8_t *payload);
void sfra_set_step_freq(uint8_t *payload);
void sfra_set_samp_freq(uint8_t *payload);
void sfra_set_input_amp(uint8_t *payload);

void sfra_ret_ack(sfra_cmd_t command);
void sfra_ret_nack(sfra_cmd_t command);
