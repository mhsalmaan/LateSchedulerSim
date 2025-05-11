#include "task.h"
#include "utils.h"
#include <iostream>
#include <algorithm>

Task::Task(int id, const std::string& data)
    : id(id), data(data), completed(false), is_speculative(false), in_progress(false), node_speed_factor(1.0) {}

void Task::markStarted() {
    start_time = std::chrono::steady_clock::now();
    in_progress = true;
    std::cout << "[" << currentTimestamp() << "] Task " << id << " started\n";
}

void Task::markCompleted() {
    end_time = std::chrono::steady_clock::now();
    completed = true;
    in_progress = false;
    std::cout << "[" << currentTimestamp() << "] Task " << id << " completed\n";
}

double Task::getDuration() const {
    return std::chrono::duration<double>(end_time - start_time).count();
}

double Task::getProgress() const {
    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - start_time).count();
    return std::min(1.0, elapsed / (1.0 / node_speed_factor));
}

double Task::getProgressRate() const {
    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - start_time).count();
    return elapsed == 0 ? 0.0 : getProgress() / elapsed;
}

double Task::getEstimatedTimeToEnd() const {
    double rate = getProgressRate();
    return rate == 0.0 ? 9999.0 : (1.0 - getProgress()) / rate;
}