#include <stddef.h>
#include <stdint.h>

/*
	This header contains functions for interacting with the
	PS/2 controller.
*/



struct ps2_status_t {
	uint8_t out_status : 1;
	uint8_t in_status  : 1;
	uint8_t sys_flag   : 1;
	uint8_t command    : 1;
	uint8_t opt_1 	   : 1;
	uint8_t opt_2 	   : 1;
	uint8_t timeout	   : 1;
	uint8_t parity	   : 1;
};


void ps2_init_controller();

void ps2_send_command(unsigned char);
void ps2_send_data(unsigned char);

struct ps2_status_t ps2_read_status();
unsigned char ps2_read_data();

void ps2_wait_input_empty();
void ps2_wait_output_full();
void ps2_flush_out_buffer();
