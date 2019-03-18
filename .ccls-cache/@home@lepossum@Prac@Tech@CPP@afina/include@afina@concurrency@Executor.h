#ifndef AFINA_CONCURRENCY_EXECUTOR_H
#define AFINA_CONCURRENCY_EXECUTOR_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>

namespace Afina {
namespace Concurrency {

/**
 * # Thread pool
 */
class Executor {
    enum class State {
        // Threadpool is fully operational, tasks could be added and get executed
        kRun,

        // Threadpool is on the way to be shutdown, no new task could be added,
        // but existing will be completed as requested
        kStopping,

        // Threadppol is stopped
        kStopped,

        // why not?
        kReady
    };

    Executor(std::string name, int size, int _low, int _high, int _time)
        : _max_queue_size(size)
        , _high_watermark(_high)
        , _low_watermark(_low)
        , _idle_time(_time)
        , state(State::kReady)
    { }

    ~Executor();


    void Start();
    /**
     * Signal thread pool to stop, it will stop accepting new jobs and close threads just after each become
     * free. All enqueued jobs will be complete.
     *
     * In case if await flag is true, call won't return until all background jobs are done and all threads are stopped
     */
    void Stop(bool await = false);

    /**
     * Add function to be executed on the threadpool. Method returns true in case if task has been placed
     * onto execution queue, i.e scheduled for execution and false otherwise.
     *
     * That function doesn't wait for function result. Function could always be written in a way to notify caller about
     * execution finished by itself
     */
    template <typename F, typename... Types> bool Execute(F &&func, Types... args) {
        // Prepare "task"
        auto exec = std::bind(std::forward<F>(func), std::forward<Types>(args)...);
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            if (state != State::kRun || tasks.size() >= _max_queue_size) {
                return false;
            } //

            // Enqueue new task
            tasks.push_back(exec);
            if (_free_threads == 0 && threads.size() >= _max_queue_size)
                return false;

            empty_condition.notify_one();
        }
        return true;
    }

private:
    // No copy/move/assign allowed
    Executor(const Executor &);            // = delete;
    Executor(Executor &&);                 // = delete;
    Executor &operator=(const Executor &); // = delete;
    Executor &operator=(Executor &&);      // = delete;

    /**
     * Main function that all pool threads are running. It polls internal task queue and execute tasks
     */
    friend void perform(Executor *executor);

    /**
     * Mutex to protect state below from concurrent modification
     */
    std::mutex mutex;

    /**
     * Conditional variable to await new data in case of empty queue
     */
    std::condition_variable empty_condition;

    /**
     * Vector of actual threads that perorm execution
     */
    std::vector<std::thread> threads;

    /**
     * Task queue
     */
    std::deque<std::function<void()>> tasks;

    /**
     * Flag to stop bg threads
     */
    State state;

    /*
     * My stuff
     */
    const int _low_watermark;
    const int _high_watermark;
    const int _max_queue_size;
    const int _idle_time;
    int _free_threads;
    std::condition_variable _cv_stopping;

    void _add_thread();
    void _erase_thread();
};

} // namespace Concurrency
} // namespace Afina

#endif // AFINA_CONCURRENCY_EXECUTOR_H
