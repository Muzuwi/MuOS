#include <Arch/x86_64/CPU.hpp>
#include <Core/Log/Logger.hpp>
#include <Daemons/Testd/Testd.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>

CREATE_LOGGER("userland_test", core::log::LogLevel::Debug);

void Testd::test_kernel_thread() {
	while(true) {
		log.debug("Thread2(pid={}, tid={}) woke up", Thread::current()->parent()->pid(), Thread::current()->tid());
		Thread::current()->msleep(2000);
	}
}

void Testd::userland_test_thread() {
	auto* current = Thread::current();

	//  Create a userland stack for the thread
	void* user_stack = current->parent()->vmm().allocate_user_stack(VMM::user_stack_size());

	const uint8 bytes[] = { 0x48, 0xB8, 0x6F, 0x72, 0x6C, 0x64, 0x21, 0x00, 0x00, 0x00, 0x50, 0x48, 0xB8, 0x48, 0x65,
		                    0x6C, 0x6C, 0x6F, 0x2C, 0x20, 0x77, 0x50, 0x48, 0x89, 0xE7, 0x48, 0xC7, 0xC0, 0xFF, 0x00,
		                    0x00, 0x00, 0x0F, 0x05, 0x48, 0xC7, 0xC0, 0x64, 0x00, 0x00, 0x00, 0x48, 0xC7, 0xC7, 0x00,
		                    0x40, 0x00, 0x00, 0x0F, 0x05, 0x48, 0xC7, 0xC3, 0xFF, 0xFF, 0xFF, 0xFF, 0x48, 0xC7, 0xC1,
		                    0x00, 0x40, 0x00, 0x00, 0x48, 0x89, 0x18, 0x48, 0x83, 0xC0, 0x08, 0x48, 0x83, 0xE9, 0x08,
		                    0x48, 0x83, 0xF9, 0x00, 0x75, 0xEF, 0x48, 0xC7, 0xC3, 0x0A, 0x00, 0x00, 0x00, 0x48, 0xC7,
		                    0xC7, 0xE8, 0x03, 0x00, 0x00, 0x48, 0xC7, 0xC0, 0xFE, 0x00, 0x00, 0x00, 0x0F, 0x05, 0x48,
		                    0xFF, 0xCB, 0x48, 0x83, 0xFB, 0x00, 0x75, 0xE7, 0x48, 0xC7, 0xC0, 0x64, 0x00, 0x00, 0x00,
		                    0x48, 0xC7, 0xC7, 0x00, 0x10, 0x00, 0x00, 0x0F, 0x05, 0x48, 0xC7, 0xC3, 0xFF, 0xFF, 0xFF,
		                    0xFF, 0x48, 0xC7, 0xC1, 0x00, 0x10, 0x00, 0x00, 0x48, 0x89, 0x18, 0x48, 0x83, 0xC0, 0x08,
		                    0x48, 0x83, 0xE9, 0x08, 0x48, 0x83, 0xF9, 0x00, 0x75, 0xEF, 0x48, 0xC7, 0xC7, 0xE8, 0x03,
		                    0x00, 0x00, 0x48, 0xC7, 0xC0, 0xFE, 0x00, 0x00, 0x00, 0x0F, 0x05, 0xEB, 0xEE };
	auto* shellcode_location = (uint8*)0x100000;

	log.debug("Thread({}): mapping shellcode", current->tid());
	auto mapping = VMapping::create((void*)shellcode_location, 0x1000, VM_READ | VM_WRITE | VM_EXEC, MAP_SHARED);
	ENSURE(current->parent()->vmm().insert_vmapping(gen::move(mapping)));

	log.debug("Thread({}): copying shellcode", current->tid());
	for(auto& b : bytes) {
		*shellcode_location = b;
		shellcode_location++;
	}

	log.debug("Thread({}): jumping to user", current->tid());
	PtraceRegs regs = PtraceRegs::user_default();
	regs.rip = 0x100000;
	regs.rsp = (uint64)user_stack;
	CPU::jump_to_user(&regs);
}
