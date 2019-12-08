#include <Arch/i386/portio.hpp>

//  Implemented @ asm/*
extern "C" void outToPortB(uint32_t, uint32_t);
extern "C" void loadGDT(uint32_t);
extern "C" uint8_t inFromPortB(uint32_t);
extern "C" uint32_t inFromPortD(uint32_t);

/*
	Writes a byte to the specified I/O port
*/
extern "C" void out(uint16_t port, uint8_t value){
	outToPortB(port, value);
}

/*
 *  Writes a dword to the specified I/O port
 */
extern "C" void outd(uint16_t port, uint32_t value) {
	asm volatile("mov %%eax, %0\n"
				 "mov %%dx, %1\n"
				 "out %%dx, %%eax\t\n"
				 :
				 :""(value), ""(port)
				 : "eax");
}

/*
	Reads a byte from the specified I/O port
*/
extern "C" uint8_t in(uint16_t port){
	return inFromPortB(port);
}

/*
	Reads a word from the specified IO port
*/
extern "C" uint32_t inw(uint16_t port){
	return inFromPortD(port);
}

/*
	Calls lgdt with given address
*/
extern "C" void lgdt(uint32_t addr){
	loadGDT(addr);
}
