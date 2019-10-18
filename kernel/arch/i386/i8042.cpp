#include <arch/i386/i8042.hpp>
#include <stdio.h>

ps2_device_t i8042::devices[I8042_MAX_DEVICES] = {};

void identify_device(unsigned char* response, size_t len){
	if(len == 1){
		switch(response[0]){
			case 0x0:
				printf("[i804_ctrl]: Device PS/2 Mouse\n");
				break;
			case 0x3:
				printf("[i8042_ctrl]: Device PS/2 Mouse w/ scrollwheel\n");
				break;
			case 0x4:
				printf("[i8042_ctrl]: Device PS/2 Mouse 5 button\n");
				break;
			default: 
				printf("[i8042_ctrl]: Unknown device id %x\n", response[0]);
		}
	} else if(len == 2){
		switch(response[0]){
			case 0xAB:
			{
				switch(response[1]){
					case 0x41:
					case 0xC1:
						printf("[i8042_ctrl]: Device MF/2 keyboard w/ translation\n");
						break;
					case 0x83:
						printf("[i8042_ctrl]: Device MF/2 keyboard\n");

					default: break;
				}	
				break;
			}			
			default: 
				printf("[i8042_ctrl]: Unknown device id %x%x\n", response[0], response[1]);
				break;	
		}
	}
}


/*
	Initializes the i8042 controller, resets devices and performs
	self checks to see which ones are present
*/
void i8042::init_controller(){
	unsigned char data;
	bool has_second_port = true;

	//  disable 1st device
	write_command(0xAD);
	// flush_out_buffer();

	//  disable 2nd device
	write_command(0xA7);

	//  Set configuration
	write_command(0x20);
	data = read_data();

	unsigned char backup_config_data = data;
	printf("[i8042_ctrl]: Received initial configuration byte %x\n", data);
	if(!(data & (1 << 5))){
		has_second_port = false;
		//  2nd port not present
	}
	data &= ~0xCB;

	//  Send configuration
	write_command(0x60);
	write_data(data);

	//  Test controller
	write_command(0xAA);
	data = read_data();
	if(data == 0x55){
		printf("[i8042_ctrl]: Self-test passed\n");
	} else if(data == 0xFC){
		printf("[i8042_ctrl]: Self-test failed\n");
	} else {
		printf("[i8042_ctrl]: Self-test likely failed (unknown response %x)\n", data);
	}

	if(data != 0x55){
		printf("[i8042_ctrl]: Restoring previous configuration\n");
		write_command(0x60);
		write_data(backup_config_data);

		//  Check if passes now
		write_command(0xAA);
		data = read_data();
		if(data != 0x55){
			printf("[i8042_ctrl]: Self test still failing, for now ignoring..\n");
			//return;
		}
	}

	//  Determine if second channel is present
	if(has_second_port){
		//  Enable second port
		write_command(0xA8);
		//  Reread cofiguration byte
		write_command(0x20);
		unsigned char conf = read_data();
		if(conf & (1 << 5)) {
			//  Absent
			has_second_port = false;
			printf("[i8042_ctrl]: 2nd channel absent\n");
		} else {
			//  Present, disable the second port again
			has_second_port = true;
			printf("[i8042_ctrl]: 2nd channel present\n");
			write_command(0xA7);
		}
	}

	//  Test PS/2 ports
	write_command(0xAB);
	data = read_data();
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
		write_command(0xA9);
		data = read_data();
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

	//  Response buffer
	unsigned char response[2];
	size_t cnt = 0;

	//  Enable working PS/2 ports
	if(first_good == true){
		printf("[i8042_ctrl]: Enabling channel 1\n");
		write_command(0xAE);
		write_data(0xFF);
		printf("[i8042_ctrl]: Channel 1 reset, self-test ");
		response[0] = read_data();
		response[1] = read_data();
		if(response[0] == 0xFA && response[1] == 0xAA){
			printf("passed\n");
		} else {
			printf("failed\n");
		}
	}

	if(second_good == true){
		printf("[i8042_ctrl]: Enabling channel 2\n");
		write_command(0xA8);
		write_command(0xD4);
		write_data(0xFF);
		printf("[i8042_ctrl]: Channel 2 reset, self-test ");
		response[0] = read_data();
		response[1] = read_data();
		if(response[0] == 0xFA && response[1] == 0xAA){
			printf("passed\n");
		} else {
			printf("failed\n");
		}
	}

	// flush_out_buffer();

	//  Check device type
	//  Device 1
	/*if(first_good == true){
		write_data(0xF5);
		while(read_data() != 0xFA){
			//printf("[i8042_ctrl]: waiting for ack %n");
			
		}
		write_data(0xF2);
		while(read_data() != 0xFA){
			//printf("[i8042_ctrl] waiting for ack\n");
		}
		cnt = 0;
		while(check_timeout() && cnt < 2){
			response[cnt++] = read_data();
		}
		identify_device(response, cnt);		
	}

	//  Device 2
	if(second_good == true){
		write_command(0xD4);
		write_data(0xF5);
		while(read_data() != 0xFA){
			// printf("[i8042_ctrl]: waiting for ack %n");
		}
		write_command(0xD4);
		write_data(0xF2);
		while(read_data() != 0xFA){
			// printf("[i8042_ctrl] waiting for ack\n");
		}
		cnt = 0;
		while(check_timeout() && cnt < 2){
			response[cnt++] = read_data();
		}
		identify_device(response, cnt);
	}*/

	if(first_good || second_good){
		printf(I8042_LOG "One or more devices have been successfully initialized\n");
	}
}

/*
	Sends data to the device port
	This function is blocking, and will wait for the input buffer
	to clear before sending
*/
extern "C" void ps2ctrl_write_data(unsigned char);
void i8042::write_data(uint8_t data){
	wait_input_empty();
	ps2ctrl_write_data(data);
}

/*
	Sends a specified command to the controller	
	This function is blocking, and will wait for the input buffer 
	to clear before sending the command
*/
extern "C" void ps2ctrl_write_command(unsigned char);
void i8042::write_command(uint8_t cmd){
	wait_input_empty();
	ps2ctrl_write_command(cmd);
}

/*
	Waits for the output buffer to fill up, then
	reads data from register 0x60 and returns it
	This function is blocking, and will wait for new data
	from the output buffer
*/
extern "C" unsigned char ps2ctrl_read();
uint8_t i8042::read_data(){
	wait_output_full();
 	return ps2ctrl_read();
}


bool i8042::read_status(){

}


/*
	Waits until bit 0 in the status register is set, i.e.
	the output buffer was filled and ready to read data from	
	This function is blocking (obviously)
*/
extern "C" void ps2ctrl_wait_output_full();
void i8042::wait_output_full(){
	ps2ctrl_wait_output_full();
}

/*
	Waits until bit 1 in the status register of the controller is set,
	i.e. the input buffer is empty	
	This function is blocking (obviously)
*/
extern "C" void ps2ctrl_wait_input_empty();
void i8042::wait_input_empty(){
	ps2ctrl_wait_input_empty();
}

/*
	Checks the timeout flag, 
	TODO: Terrible, replace this
*/
extern "C" int ps2ctrl_timeout_error();
bool i8042::check_timeout(){
	return (ps2ctrl_timeout_error() >= 0);
}