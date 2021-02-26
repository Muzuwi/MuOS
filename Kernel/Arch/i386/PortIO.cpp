#include <Arch/i386/PortIO.hpp>

extern "C" void loadGDT(uint32_t);

/*
	Writes a byte to the specified I/O port
*/
extern "C" void out(uint16_t port, uint8_t value){
	asm volatile("mov %%al, %0\n"
	             "mov %%dx, %1\n"
	             "out %%dx, %%al\t\n"
	             :
	             :""(value), ""(port)
	             : "eax");
}

/*
	Writes a word to the specified I/O port
*/
extern "C" void outw(uint16_t port, uint16_t value){
	asm volatile("mov %%ax, %0\n"
	             "mov %%dx, %1\n"
	             "out %%dx, %%ax\t\n"
	:
	:""(value), ""(port)
	: "eax");
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
	uint32_t data = 0;
	asm volatile("mov %%dx, %1\n"
	             "in %%al, %%dx\n"
	             "mov %0, %%eax\t\n"
	             : "=r"(data)
	             : ""(port)
	             : "memory"
	             );
	return data;
}

/*
	Reads a dword from the specified IO port
*/
extern "C" uint32_t ind(uint16_t port){
	uint32_t data = 0;
	asm volatile("mov %%dx, %1\n"
	             "in %%eax, %%dx\n"
	             "mov %0, %%eax\t\n"
	             : "=r"(data)
	             : ""(port)
	             : "memory"
	             );
	return data;
}

/*
	Reads a word from the specified I/O port
*/
extern "C" uint16_t inw(uint16_t port) {
	uint16_t data = 0;
	asm volatile("mov %%dx, %1\n"
	             "in %%ax, %%dx\n"
	             "mov %0, %%ax\t\n"
				: "=r"(data)
				: ""(port)
				: "memory"
				);
	return data;
}

void wrmsr(uint32_t ecx, uint64_t value) {
	uint32_t eax = value&0xffffffffu,
			 edx = value>>32u;
	asm volatile("wrmsr"
	:
	:"c"(ecx), "a"(eax), "d"(edx));
}

uint64_t rdmsr(uint32_t ecx) {
	uint32_t eax = 0, edx = 0;
	asm volatile("rdmsr"
	: "=a"(eax), "=d"(edx)
	: "c"(ecx));
	return (static_cast<uint64_t>(edx)<<32u) | (static_cast<uint64_t>(eax));
}
