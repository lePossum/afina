#include <afina/concurrency/Executor.h>

namespace Afina {
namespace Concurrency {

    void perform(Executor *executor) {
        while (executor->state == Executor::State::kRun) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(executor->mutex);
                auto time_until = std::chrono::system_clock::now() + std::chrono::milliseconds(executor->_idle_time);
                while (executor->tasks.empty() && executor->state == Executor::State::kRun) {
                    executor->_free_threads++;
                    if (executor->empty_condition.wait_until(lock, time_until) == std::cv_status::timeout) {
                        if (executor->threads.size() > executor->_low_watermark) {
                            executor->_erase_thread();
                            return;
                        }
                        else {
                            executor->empty_condition.wait(lock);
                        }
                    }
                    executor->_free_threads--;
                }
                if (executor->tasks.empty()) {
                    continue;
                }
                task = executor->tasks.front();
                executor->tasks.pop_front();
            }
            task();
        }
        {
            std::unique_lock<std::mutex> lock(executor->mutex);
            executor->_erase_thread();
            if (executor->threads.empty()) {
                executor->_cv_stopping.notify_all();
            }
        }
    }

    void Executor::Start() {
        std::unique_lock<std::mutex> lock(this->mutex);
        for (int i = 0; i < _low_watermark; i++) {
            threads.push_back(std::thread(&perform, this));
        }
        _free_threads = 0;
        state = State::kRun;
    }

    void Executor::Stop(bool await) {
        state = State::kStopping;
        empty_condition.notify_all();
        {
            std::unique_lock<std::mutex> _lock(mutex);
            while (!threads.empty()) {
                _cv_stopping.wait(_lock);
            }
        }
        state = State::kStopped;
    }

    void Executor::_add_thread() {
        threads.push_back(std::thread(&perform, this));
    }

    void Executor::_erase_thread() {
        std::thread::id this_id = std::this_thread::get_id();
        auto iter = std::find_if(threads.begin(), threads.end(), [=](std::thread &t) { return (t.get_id() == this_id); });
        if (iter != threads.end()) {
            iter->detach();
            _free_threads--;
            threads.erase(iter);
            return;
        }
    throw std::runtime_error("error while erasing thread");
    }
}
} // namespace Afina
