#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

class Executor {
// два потока: один записывает в _to_execute, второй из него читает
public:
	Executor() : _to_execute(nullptr) { } // плохо

	void ExecutePlease(std::function *func)
	{
		std::unique_lock<std::mutex> _lock(_mutex);
		while (_to_execute != nullptr) {
			_has_space.wait(_lock);
		}

		_to_execute = func;
		_has_execute.notify_one()
	}

	void Start()
	{
		_thread = std::thread(&Executor::Run, this)
	}

	void Stop()
	{
		// throw Exception;
		// KAVOOO
	}
protected:
	void Run() // глобальный lock = в один момент времени исполнение только одной задачи
	{
		for (;;){
			std::function tmp;
			{
				tmp = nullptr;
				std::unique_lock<std::mutex> _lock(mutex);
				while (_to_execute == nullptr) {
					_has_execute.wait(_lock);
				}
				std::swap(tmp, _to_execute);
				_has_space.notify_all();
			}
			(*tmp)();
			//  } catch Exception => break
		}
	}

private:

	std::thread _thread;

	std::mutex _mutex;
	std::condition_variable _has_execute;
	std::condition_variable _has_space;
	std::function *_to_execute;
};