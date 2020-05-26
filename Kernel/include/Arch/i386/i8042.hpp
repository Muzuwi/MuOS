#pragma once
#include <stdint.h>
#include <stddef.h>

//struct ps2_status_t {
//	uint8_t out_status : 1;
//	uint8_t in_status  : 1;
//	uint8_t sys_flag   : 1;
//	uint8_t command    : 1;
//	uint8_t opt_1 	   : 1;
//	uint8_t opt_2 	   : 1;
//	uint8_t timeout	   : 1;
//	uint8_t parity	   : 1;
//};

/*
	This header is for the i8042 PS/2 kernel driver
	It allows for writing/reading data from/to the 
	controller and connected devices
*/
namespace i8042 {
	void init_controller();

	//  Writing data
	void write_data(uint8_t);
	void write_command(uint8_t);

	//  Reading data
	uint8_t read_data();
	uint8_t read_data_timeout();
	bool read_status();

	//  Misc functions
	void wait_output_full();
	void wait_input_empty();

	bool check_timeout();

}