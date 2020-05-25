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
	static void yield(TrapFrame frame);
	static void notify_new_process(Process*);
};