#include "Idle.hpp"

void Idle::idle_thread() {
	while(true) {
		asm volatile("hlt");
	}
}
