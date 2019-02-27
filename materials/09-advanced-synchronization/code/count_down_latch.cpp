#include "count_down_latch.h"

countdownlatch::countdownlatch(uint32_t count) { this->count = count; }

void countdownlatch::await(uint64_t nanosecs) {
    std::unique_lock<std::mutex> lck(lock); // add while for suspicious wakeup
    if (0 == count) {                       // посчитать, сколько ждать до ? точки во времени ?
        return;                             // вместо первой проверки будет цикл
    }
    // while (count != 0) wait
    // if (nanosecs > 0) выйти из метода, поэтому его нет в while
    // 1) вышли из-за нотифай 2) из-за суспишс... 3) count != 0
    // из-за 2) можно прождать вечность
    if (nanosecs > 0) {
        cv.wait_for(lck, std::chrono::nanoseconds(nanosecs)); // прочитать про wait_for на cppref
    } else {
        cv.wait(lck); // передать функцию ? булевскую ? и сюда и в wait_for, всё решит
    }
}

uint32_t countdownlatch::get_count() {
    std::unique_lock<std::mutex> lck(lock);
    return count;
}

void countdownlatch::count_down() {
    std::unique_lock<std::mutex> lck(lock);
    if (0 == count) {
        return;
    }
    --count;
    if (0 == count) {
        cv.notify_all();
    }
}
