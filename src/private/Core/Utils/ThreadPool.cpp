#include "Core/Utils/ThreadPool.h"

ThreadPool::ThreadPool(uint32_t nNumThreads) {
	for(uint32_t i = 0; i < nNumThreads; ++i) {
		this->m_workers.emplace_back([this] {
			while (true) {
				std::function<void()> task;

				{
					std::unique_lock<std::mutex> lock(this->m_queueMutex);

					this->m_condition.wait(lock, [this] {
						return this->m_stop || !this->m_tasks.empty();
					});

					if (this->m_stop && this->m_tasks.empty()) {
						return;
					}
					
					task = std::move(this->m_tasks.front());
					this->m_tasks.pop();
				}

				task();

				{
					this->m_activeTasks--;
				}

				this->m_doneCondition.notify_all();
			}
		});
	}
}

ThreadPool::~ThreadPool() {
	{
		std::unique_lock<std::mutex> lock(this->m_queueMutex);
		this->m_stop = true;
	}

	this->m_condition.notify_all();

	for (std::thread& worker : this->m_workers) {
		if (worker.joinable())
			worker.join();
	}
}

/**
* Wait for all tasks to complete
*/
void
ThreadPool::WaitAll() {
	std::unique_lock<std::mutex> lock(this->m_queueMutex);
	this->m_doneCondition.wait(lock, [this] {
		return this->m_tasks.empty() && this->m_activeTasks == 0;
	});
}