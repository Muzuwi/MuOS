#pragma once
#include <LibGeneric/List.hpp>
#include <Locks/KSemaphore.hpp>
#include <Process/Thread.hpp>

class MigrationService {
	Thread* m_migration_thread {};
	gen::List<Thread*> m_incoming_threads {};
	gen::Spinlock m_service_lock {};//  Protects incoming thread list
	KSemaphore m_incoming_semaphore {};

	void do_migrate_thread(Thread*);
	void migration_thread_main();
	static void migration_thread_entrypoint();
public:
	MigrationService();

	void migrate(Thread* incoming);
};