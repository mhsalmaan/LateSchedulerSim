#ifndef TASK_H
#define TASK_H

#include <string>
#include <atomic>
#include <chrono>

class Task {
public:
    int id;
    std::string data;
    std::atomic<bool> completed;
    std::atomic<bool> is_speculative;
    std::atomic<bool> in_progress;
    double node_speed_factor;

    Task(int id, const std::string& data);
    void markStarted();
    void markCompleted();
    double getProgress() const;
    double getProgressRate() const;
    double getEstimatedTimeToEnd() const;
    double getDuration() const;

private:
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
};

#endif // TASK_H