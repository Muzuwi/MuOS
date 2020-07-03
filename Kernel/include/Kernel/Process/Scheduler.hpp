#pragma once
#include <include/Arch/i386/TrapFrame.hpp>
#include <Kernel/Process/Process.hpp>

enum class SchedulerAction {
	Use,
	PickAgain
};

class Scheduler {
	static Process* pick_next();
	static SchedulerAction handle_process_pick(Process*);
public:
	static void initialize();
	static void enter_scheduler_loop();
	static void notify_new_process(Process*);

	static void yield_with_irq_frame(uint32_t esp);
	static void switch_task();
};