#include <kernel/io/ps2.h>
#include <stdio.h>
#include <stdbool.h>

/*
	Initializes the controller on boot
*/
void ps2_init_controller(){
	unsigned char data;
	int has_second_port = 0;

	//  disable 1st device
	ps2_send_command(0xAD);
	ps2_flush_out_buffer();

	//  disable 2nd device
	ps2_send_command(0xA7);

	//  Set configuration
	ps2_send_command(0x20);
	data = ps2_read_data();

	unsigned char backup_config_data = data;
	printf("[i8042_ctrl]: Received initial configuration byte %x\n", data);
	if(!(data & (1 << 5))){
		has_second_port = -1;
		//  2nd port not present
	}
	data &= ~0xCB;

	//  Send configuration
	ps2_send_command(0x60);
	ps2_send_data(data);

	//  Test controller
	ps2_send_command(0xAA);
	data = ps2_read_data();
	printf("[i8042_ctrl]: Self-test response %x\n", data);
	if(data == 0x55){
		printf("[i8042_ctrl]: Self-test passed\n");
	} else if(data == 0xFC){
		printf("[i8042_ctrl]: Self-test failed\n");
	} else {
		printf("[i8042_ctrl]: Self-test likely failed (unknown response %x)\n", data);
	}

	if(data != 0x55){
		printf("[i8042_ctrl]: Restoring previous configuration\n");
		ps2_send_command(0x60);
		ps2_send_data(backup_config_data);

		//  Check if passes now
		ps2_send_command(0xAA);
		data = ps2_read_data();
		if(data != 0x55){
			printf("[i8042_ctrl]: Self test still failing, for now ignoring..\n");
			//return;
		}
	}

	//  Determine if second channel is present
	if(has_second_port != -1){
		//  Enable second port
		ps2_send_command(0xA8);
		//  Reread cofiguration byte
		ps2_send_command(0x20);
		unsigned char conf = ps2_read_data();
		if(conf & (1 << 5)) {
			//  Absent
			has_second_port = -1;
			printf("[i8042_ctrl]: 2nd channel absent\n");
		} else {
			//  Present, disable the second port again
			has_second_port = 1;
			printf("[i8042_ctrl]: 2nd channel present\n");
			ps2_send_command(0xA7);
		}
	}

	//  Test PS/2 ports
	ps2_send_command(0xAB);
	data = ps2_read_data();
	bool first_good = false, second_good = false;

	switch(data){
		case 0x0:{
			printf("[i8042_ctrl]: Port 1 test passed\n");
			first_good = true;
			break;
		}
		case 0x1:{
			printf("[i8042_ctrl]: Port 1 failure - clock line stuck low\n");
			break;
		}
		case 0x2:{
			printf("[i8042_ctrl]: Port 1 failure - clock line stuck high\n");
			break;
		}
		case 0x3:{
			printf("[i8042_ctrl]: Port 1 failure - data line stuck low\n");
			break;
		}
		case 0x4:{
			printf("[i8042_ctrl]: Port 1 failure - data line stuck high\n");
			break;
		}
		default:{
			printf("[i8042_ctrl]: Port 1 test - unknown response\n");
			break;
		}
	}

	if(has_second_port == 1){
		ps2_send_command(0xA9);
		data = ps2_read_data();
		switch(data){
			case 0x0:{
				printf("[i8042_ctrl]: Port 2 test passed\n");
				second_good = true;
				break;
			}
			case 0x1:{
				printf("[i8042_ctrl]: Port 2 failure - clock line stuck low\n");
				break;
			}
			case 0x2:{
				printf("[i8042_ctrl]: Port 2 failure - clock line stuck high\n");
				break;
			}
			case 0x3:{
				printf("[i8042_ctrl]: Port 2 failure - data line stuck low\n");
				break;
			}
			case 0x4:{
				printf("[i8042_ctrl]: Port 2 failure - data line stuck high\n");
				break;
			}
			default:{
				printf("[i8042_ctrl]: Port 2 test - unknown response\n");
				break;
			}
		}
	}

	if(!(first_good || second_good)){
		printf("[i8042_ctrl]: No working channels, bailing out..\n");
	}

	//  Enable working PS/2 ports
	if(first_good == true){
		printf("[i8042_ctrl]: Enabling channel 1\n");
		ps2_send_command(0xAE);
		ps2_send_data(0xFF);
		printf("[i8042_ctrl]: Channel 1 reset response %x%x\n", ps2_read_data(), ps2_read_data());
	}

	if(second_good == true){
		printf("[i8042_ctrl]: Enabling channel 2\n");
		ps2_send_command(0xA8);
		ps2_send_command(0xD4);
		ps2_send_data(0xFF);
		printf("[i8042_ctrl]: Channel 2 reset response %x%x\n", ps2_read_data(), ps2_read_data());
	}

}


/*
	Sends a specified command to the controller	
*/
void ps2_send_command(unsigned char cmd){
	ps2_wait_input_empty();
	extern void ps2ctrl_write_command(unsigned char);
	ps2ctrl_write_command(cmd);
	//printf("[i8042_ctrl]: sent command %x\n", cmd);
}


void ps2_send_data(unsigned char data){
	ps2_wait_input_empty();
	extern void ps2ctrl_write_data(unsigned char);
	ps2ctrl_write_data(data);
	//'printf("[i8042_ctrl]: sent data %x\n", data);
}


/*
	Reads the status register contents and returns a struct
	containing the current status
	FIXME: This is broken right now
*/
struct ps2_status_t ps2_read_status(){
	ps2_wait_output_full();
	extern void ps2ctrl_read_status(uint8_t* val);
	uint8_t temp;
	ps2ctrl_read_status(&temp);
}


/*
	Reads data from register 0x60 and returns it
*/
unsigned char ps2_read_data(){
	ps2_wait_output_full();
	extern unsigned char ps2ctrl_read();
 	return ps2ctrl_read();
}


/*
	Waits until bit 1 in the status register of the controller is set,
	i.e. the input buffer is empty	
*/
void ps2_wait_input_empty(){
	extern void ps2ctrl_wait_input_empty();
	ps2ctrl_wait_input_empty();
}


/*
	Waits until bit 0 in the status register is set, i.e.
	the output buffer was filled and ready to read data from	
*/
void ps2_wait_output_full(){
	extern void ps2ctrl_wait_output_full();
	ps2ctrl_wait_output_full();
}

/*
	Flushes out the contents of the output buffer
	on the ps/2 controller	
*/
void ps2_flush_out_buffer(){
	extern void ps2ctrl_flush();
	ps2ctrl_flush();
}