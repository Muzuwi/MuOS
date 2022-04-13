#include "MigrationService.hpp"
#include <LibGeneric/LockGuard.hpp>
#include <Process/Process.hpp>
#include <SMP/SMP.hpp>

MigrationService::MigrationService() {
	auto thread = Process::create_with_main_thread("migrationd[]", Process::kerneld(), migration_thread_entrypoint);
	m_migration_thread = thread.get();
}

void MigrationService::migration_thread_entrypoint() {
	this_cpu().migration()->migration_thread_main();
	ASSERT_NOT_REACHED();
}

void MigrationService::migrate(Thread* incoming) {
	gen::LockGuard<gen::Spinlock> guard { m_service_lock };
	m_incoming_threads.push_back(incoming);
	m_incoming_semaphore.signal();
}

void MigrationService::migration_thread_main() {
	while(true) {
		m_incoming_semaphore.wait();
		{
			gen::LockGuard<gen::Spinlock> guard { m_service_lock };
			while(!m_incoming_threads.empty()) {
				auto thread = m_incoming_threads.front();
				m_incoming_threads.pop_front();
				do_migrate_thread(thread);
			}
		}
	}
}

void MigrationService::do_migrate_thread(Thread* thread) {
	if(!thread) {
		return;
	}
	klogf("[Migration(VID={})] Migrating thread TID={}\n", thread->tid());
	//	this_cpu().scheduler().add_thread_to_rq(thread);
}
