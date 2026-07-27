#ifndef BARE_METAL_SCHEDULER_H
#define BARE_METAL_SCHEDULER_H

#include "bare_metal/task.h"

namespace bare_metal {

template <size_t kMaxNumTasks = 8U>
class Scheduler {
public:
    Scheduler() = default;
    ~Scheduler() noexcept = default;

    bool addTask(PeriodicTask* task) noexcept {
        if (num_tasks_ >= kMaxNumTasks) {
            return false;
        }
        tasks_[num_tasks_] = task;
        num_tasks_++;
        return true;
    }

    void start() noexcept {
        for (size_t i = 0U; i < num_tasks_; i++) {
            if (tasks_[i] == nullptr) {
                continue;
            }
            tasks_[i]->start();
        }
    }

    __attribute__((noreturn))
    void run() noexcept {
        while (true) {
            for (size_t i = 0U; i < num_tasks_; i++) {
                if (tasks_[i] == nullptr) {
                    continue;
                }
                tasks_[i]->poll();
            }
        }
    }
private:
    std::array<PeriodicTask*, kMaxNumTasks> tasks_{};
    size_t num_tasks_{0U};
};
} // namespace bare_metal
#endif // BARE_METAL_SCHEDULER_H