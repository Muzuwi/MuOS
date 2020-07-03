#include <Arch/i386/i8042.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Arch/i386/PortIO.hpp>
#include <Arch/i386/IRQDisabler.hpp>

void identify_device(unsigned char* response, size_t len){
	if(len == 1){
		switch(response[0]){
			case 0x0:
				kdebugf("[i804_ctrl]: Device PS/2 Mouse\n");
				break;
			case 0x3:
				kdebugf("[i8042_ctrl]: Device PS/2 Mouse w/ scrollwheel\n");
				break;
			case 0x4:
				kdebugf("[i8042_ctrl]: Device PS/2 Mouse 5 button\n");
				break;
			default:
				kdebugf("[i8042_ctrl]: Unknown device id %x\n", response[0]);
		}
	} else if(len == 2){
		switch(response[0]){
			case 0xAB:
			{
				switch(response[1]){
					case 0x41:
					case 0xC1:
						kdebugf("[i8042_ctrl]: Device MF/2 keyboard w/ translation\n");
						break;
					case 0x83:
						kdebugf("[i8042_ctrl]: Device MF/2 keyboard\n");

					default: break;
				}	
				break;
			}			
			default: 
				kdebugf("[i8042_ctrl]: Unknown device id %x%x\n", response[0], response[1]);
				break;	
		}
	}
}


/*
	Initializes the i8042 controller, resets devices and performs
	self checks to see which ones are present
*/
void i8042::init_controller(){
	IRQDisabler disabler;

	unsigned char data;
	bool has_second_port = true;

	//  disable 1st device
	write_command(0xAD);

	//  disable 2nd device
	write_command(0xA7);

	//  Flush any existing data
	while(in(0x64) & 1u)
		(void)in(0x60);

	//  Set configuration
	write_command(0x20);
	data = read_data();

	unsigned char backup_config_data = data;
	kdebugf("[i8042_ctrl]: Received initial configuration byte %x\n", data);
	if(!(data & (1 << 5))){
		has_second_port = false;
		//  2nd port not present
	}
	data &= ~0xCB;
	data |= 1;
	if(has_second_port)
		data |= 2;

	//  Send configuration
	write_command(0x60);
	write_data(data);

	//  Test controller
	write_command(0xAA);
	data = read_data();
	if(data == 0x55){
		kdebugf("[i8042_ctrl]: Self-test passed\n");
	} else if(data == 0xFC){
		kdebugf("[i8042_ctrl]: Self-test failed\n");
	} else {
		kdebugf("[i8042_ctrl]: Self-test likely failed (unknown response %x)\n", data);
	}

	if(data != 0x55){
		kdebugf("[i8042_ctrl]: Restoring previous configuration\n");
		write_command(0x60);
		write_data(backup_config_data);

		//  Check if passes now
		write_command(0xAA);
		data = read_data();
		if(data != 0x55){
			kdebugf("[i8042_ctrl]: Self test still failing, for now ignoring..\n");
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
			kdebugf("[i8042_ctrl]: 2nd channel absent\n");
		} else {
			//  Present, disable the second port again
			has_second_port = true;
			kdebugf("[i8042_ctrl]: 2nd channel present\n");
			write_command(0xA7);
		}
	}

	//  Test PS/2 ports
	write_command(0xAB);
	data = read_data();
	bool first_good = false, second_good = false;

	switch(data){
		case 0x0:{
			kdebugf("[i8042_ctrl]: Port 1 test passed\n");
			first_good = true;
			break;
		}
		case 0x1:{
			kdebugf("[i8042_ctrl]: Port 1 failure - clock line stuck low\n");
			break;
		}
		case 0x2:{
			kdebugf("[i8042_ctrl]: Port 1 failure - clock line stuck high\n");
			break;
		}
		case 0x3:{
			kdebugf("[i8042_ctrl]: Port 1 failure - data line stuck low\n");
			break;
		}
		case 0x4:{
			kdebugf("[i8042_ctrl]: Port 1 failure - data line stuck high\n");
			break;
		}
		default:{
			kdebugf("[i8042_ctrl]: Port 1 test - unknown response\n");
			break;
		}
	}

	if(has_second_port == 1){
		write_command(0xA9);
		data = read_data();
		switch(data){
			case 0x0:{
				kdebugf("[i8042_ctrl]: Port 2 test passed\n");
				second_good = true;
				break;
			}
			case 0x1:{
				kdebugf("[i8042_ctrl]: Port 2 failure - clock line stuck low\n");
				break;
			}
			case 0x2:{
				kdebugf("[i8042_ctrl]: Port 2 failure - clock line stuck high\n");
				break;
			}
			case 0x3:{
				kdebugf("[i8042_ctrl]: Port 2 failure - data line stuck low\n");
				break;
			}
			case 0x4:{
				kdebugf("[i8042_ctrl]: Port 2 failure - data line stuck high\n");
				break;
			}
			default:{
				kdebugf("[i8042_ctrl]: Port 2 test - unknown response\n");
				break;
			}
		}
	}

	if(!(first_good || second_good)){
		kdebugf("[i8042_ctrl]: No working channels, bailing out..\n");
	}

	//  Response buffer
	unsigned char response[2];
	size_t cnt = 0;

	//  Enable working PS/2 ports
	if(first_good){
		kdebugf("[i8042_ctrl]: Enabling channel 1\n");
		write_command(0xAE);
		write_data(0xFF);
		kdebugf("[i8042_ctrl]: Channel 1 reset, self-test ");
		response[0] = read_data();
		response[1] = read_data();
		if(response[0] == 0xFA && response[1] == 0xAA){
			kdebugf("passed\n");
		} else {
			kdebugf("failed\n");
		}
	}

	if(second_good == true){
		kdebugf("[i8042_ctrl]: Enabling channel 2\n");
		write_command(0xA8);
		write_command(0xD4);
		write_data(0xFF);
		kdebugf("[i8042_ctrl]: Channel 2 reset, self-test ");
		response[0] = read_data();
		response[1] = read_data();
		if(response[0] == 0xFA && response[1] == 0xAA){
			kdebugf("passed\n");
		} else {
			kdebugf("failed\n");
		}
	}

	//  Check device type
	//  Device 1
	if(first_good){
		write_data(0xF5);
		while(read_data() != 0xFA)
			;
		write_data(0xF2);
		while(read_data() != 0xFA)
			;
		
		cnt = 0;
		while(!check_timeout() && cnt < 2){  
			response[cnt++] = read_data();
		}
		identify_device(response, cnt);
	}

	//  Device 2
	if(second_good){
		write_command(0xD4);
		write_data(0xF5);
		while(read_data() != 0xFA);
		write_command(0xD4);
		write_data(0xF2);

		while(read_data() != 0xFA)
			;
		cnt = 0;
		while(check_timeout() && cnt < 2){
			response[cnt++] = read_data();
		}
		identify_device(response, cnt);
	}

	if(first_good){
		kdebugf("[i8042_ctrl] Reenabling scan mode for device 1\n");
		write_data(0xF4);
		while(read_data() != 0xFA)
			;
	}

	if(second_good){
		kdebugf("[i8042_ctrl] Reenabling scan mode for device 2\n");
		write_command(0xD4);
		write_data(0xF4);
		while(read_data() != 0xFA)
			;
	}

	if(first_good || second_good){
		kdebugf("[i8042_ctrl] One or more devices have been successfully initialized\n");
	}
}

/*
	Sends data to the device port
	This function is blocking, and will wait for the input buffer
	to clear before sending
*/
void i8042::write_data(uint8_t data){
	while(in(0x64) & 2)
		;
	out(0x60, data);
}

/*
	Sends a specified command to the controller	
	This function is blocking, and will wait for the input buffer 
	to clear before sending the command
*/
void i8042::write_command(uint8_t cmd){
	while(in(0x64) & 2)
		;
	out(0x64, cmd);
}

/*
	Waits for the output buffer to fill up, then
	reads data from register 0x60 and returns it
	This function is blocking, and will wait for new data
	from the output buffer
*/
uint8_t i8042::read_data(){  
 	while(!(in(0x64) & 1))
		;
	return in(0x60);
}


bool i8042::read_status(){
	return false;
}


/*
	Waits until bit 0 in the status register is set, i.e.
	the output buffer was filled and ready to read data from	
	This function is blocking (obviously)
*/
void i8042::wait_output_full(){
	while(!(in(0x64) & 1))
		;
}

/*
	Waits until bit 1 in the status register of the controller is set,
	i.e. the input buffer is empty	
	This function is blocking (obviously)
*/
void i8042::wait_input_empty(){
	while(in(0x64) & 2)
		;
}

/*
	Checks the timeout flag, 
	TODO: Terrible, replace this
*/
bool i8042::check_timeout(){
	return in(0x64) & (1 << 6);
}