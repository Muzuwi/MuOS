#include <LibGeneric/Algorithm.hpp>
#include <Process/Process.hpp>
#include <Scheduler/RunQueue.hpp>
#include "Arch/x86_64/IRQDisabler.hpp"

bool RunQueue::PriorityComparator::operator()(Thread* const& lhs, Thread* const& rhs) {
	return lhs->priority() <= rhs->priority();
}

Thread* RunQueue::find_runnable() const {
	if(m_active->empty()) {
		return nullptr;
	}
	return m_active->front();
}

void RunQueue::add_inactive(Thread* thread) {
	m_inactive->push(thread);
}

void RunQueue::remove_active(Thread* thread) {
	if(auto it = gen::find(*m_active, thread); it != (*m_active).end()) {
		m_active->erase(it);
	}
}

RunQueue::RunQueue()
    : m_active(&m_first)
    , m_inactive(&m_second) {}

void RunQueue::swap() {
	gen::swap(m_active, m_inactive);
}

void RunQueue::dump_statistics() {
	IRQDisabler irq_disabler {};
	klogf_static("Active queue:\n");
	for(auto const& thread : *m_active) {
		klogf_static("Thread{{{}}}, TID{{{}}}, Priority{{{}}}, Quant{{{}}}\n", Format::ptr(&thread), thread->tid(),
		             thread->priority(), thread->sched_ctx().quants_left);
	}
	klogf_static("Inactive queue:\n");
	for(auto const& thread : *m_inactive) {
		klogf_static("Thread{{{}}}, TID{{{}}}, Priority{{{}}}, Quant{{{}}}\n", Format::ptr(&thread), thread->tid(),
		             thread->priority(), thread->sched_ctx().quants_left);
	}
}
