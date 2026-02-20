#pragma once
#include "Core/Containers.h"
#include "Utils.h"

#include <thread>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>

class ThreadPool {
public:
	using Ptr = Ref<ThreadPool>;

	explicit ThreadPool(uint32_t nNumThreads = std::thread::hardware_concurrency());
	~ThreadPool();

	/**
	* Submit a task and get the future for the result 
	*/
	template<typename F, typename... Args>
	auto Submit(F&&, Args&&... args) -> std::future<typename std::invoke_result_t<F, Args...>>;

	void WaitAll();

	/**
	* Get number of worker threads
	* 
	* @returns Thread count
	*/
	uint32_t GetThreadCount() const { return this->m_workers.size(); }

	static Ptr
	CreateShared(uint32_t nNumThreads = std::thread::hardware_concurrency()) {
		return CreateRef<ThreadPool>(nNumThreads);
	}

private:
	Vector<std::thread> m_workers;
	Vector<std::future<void>> m_pendingTasks;
	std::queue<std::function<void()>> m_tasks;

	std::mutex m_queueMutex;
	std::condition_variable m_condition;
	std::atomic<bool> m_stop{ false };
	std::atomic<uint32_t> m_activeTasks{ 0 };
	std::condition_variable m_doneCondition;
};

template<typename F, typename... Args>
auto 
ThreadPool::Submit(F&& f, Args&&... args) -> std::future<typename std::invoke_result_t<F, Args...>> {
	using ReturnType = typename std::invoke_result_t<F, Args...>;

	auto task = std::make_shared<std::packaged_task<ReturnType()>>(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...)
	);

	std::future<ReturnType> result = task->get_future();
	
	{
		std::unique_lock<std::mutex> lock(this->m_queueMutex);
		if (this->m_stop) {
			Logger::Error("ThreadPool::Submit: Cannot submit task to stopped ThreadPool");
			throw std::runtime_error("ThreadPool::Submit: Cannot submit task to stopped ThreadPool");
		}

		this->m_tasks.emplace([task]() { (*task)(); });
		this->m_activeTasks++;
	}

	this->m_condition.notify_one();

	return result;
}