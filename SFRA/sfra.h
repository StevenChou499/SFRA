#include <stdint.h>

#define IRQ_SAMP_FREQ   (100e3f)
#define MAX_POINTS      (20U)

typedef enum tx_rx_state {
	WAIT_HEAD,
	WAIT_PAYLOAD,
	REPLY_CMD,
} tx_rx_state_t;

typedef enum sfra_cmd {
	NO_CMD,
	SFRA_RESET,
	START_SWEEP,
	GET_STATUS,
	GET_BODE,
} sfra_cmd_t;

typedef enum sfra_state {
	IDLE,
	GEN_TABLE,
	SWEEPING,
	SWEEP_DONE,
	SFRA_DONE,
} sfra_state_t;

uint8_t tx_cmd_table[][] = {
		{ 0xBB, 0x00, },    // No use
		{ 0xBB, 0x01, },    // SFRA_RESET ACK
		{ 0xBB, 0x02, },    // START_SWEEP ACK
		{ 0xBB, 0x03, },    // GET_STATUS ACK
		{ 0xBB, 0x04, },    // GET_BODE ACK
};

typedef struct sfra_st {
	float mag_in[MAX_POINTS];       // Input magnitude
	float pha_in[MAX_POINTS];       // Input phase
	float mag_out[MAX_POINTS];      // Output magnitude
	float pha_out[MAX_POINTS];      // Output phase
	float real_part;                // Real part of the complex for calculation
	float imag_part;                // Imaginary part of the complex for calculation
	float freq_start;               // Starting sweeping frequency
	float freq_step;                // Frequency between every sweep
	uint32_t freq_points;           // Total sweeping frequency points
	float freq_table[MAX_POINTS];   // All the sweeping frequencies in table
	uint32_t current_freq_index;    // Current sweeping frequency index
	float current_angle;            // SFRA current sweeping angle
	float inject_amplitude;         // Input amplitude for every sweep
	uint32_t input_count;           // Number of points injected in a single sweep
	uint32_t output_count;          // Number of points collected in a single sweep
	uint32_t total_count;           // Number of points total needed in a single sweep
	float sampling_freq_Hz;         // SFRA's sampling frequency
	tx_rx_state_t rx_state;         // SFRA's RX command state
	sfra_cmd_t received_cmd;        // SFRA's received command
	sfra_state_t current_state;     // SFRA's current state
} sfra_t;

extern sfra_t sfra;
extern uint8_t tx_buffer[200];
extern uint8_t rx_buffer[20];
extern uint32_t tx_len;
extern uint8_t rx_len;

uint8_t sfra_init(float sampling_rate_Hz,
	              float freq_start,
				  float freq_step,
				  float input_amplitude);
uint32_t sfra_get_sample_count(float sampling_rate_Hz, float target_freq_Hz);
float sfra_inject(float input);
void sfra_collect(float *output);
void sfra_update(void);
