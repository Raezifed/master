#include "std_include.hpp"
#include "scheduler.hpp"

void scheduler::schedule(std::function<bool()> callback, const std::chrono::milliseconds delay)
{
	task task;
	task.handler = std::move(callback);
	task.interval = delay;
	task.last_call = std::chrono::high_resolution_clock::now();

	this->add_task(std::move(task));
}

void scheduler::loop(const std::function<void()>& callback, const std::chrono::milliseconds delay)
{
	this->schedule([callback]()
	{
		callback();
		return cond_continue;
	}, delay);
}

void scheduler::once(const std::function<void()>& callback, const std::chrono::milliseconds delay)
{
	this->schedule([callback]()
	{
		callback();
		return cond_end;
	}, delay);
}

void scheduler::run_frame()
{
	std::lock_guard<std::recursive_mutex> _{this->mutex_};
	this->merge_new_tasks();

	const auto _x = gsl::finally([&]
	{
		this->is_executing_ = false;
	});
	this->is_executing_ = true;
	this->run_pending_tasks_internal();
}

void scheduler::add_task(task&& task)
{
	std::lock_guard<std::recursive_mutex> _{this->mutex_};
	this->new_tasks_.emplace_back(std::move(task));
}

void scheduler::merge_new_tasks()
{
	std::lock_guard<std::recursive_mutex> _{this->mutex_};

	if(is_executing_)
	{
		return;
	}

	this->tasks_.insert(this->tasks_.end(), this->new_tasks_.begin(), this->new_tasks_.end());
	this->new_tasks_.clear();
}

void scheduler::run_pending_tasks_internal()
{
	for (auto i = this->tasks_.begin(); i != this->tasks_.end();)
	{
		const auto now = std::chrono::high_resolution_clock::now();
		const auto diff = now - i->last_call;

		if (diff < i->interval) continue;

		i->last_call = now;

		const auto res = i->handler();
		if (res == cond_end)
		{
			i = this->tasks_.erase(i);
		}
		else
		{
			++i;
		}
	}
}
