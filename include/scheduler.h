#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <vector>
#include <memory>
#include <mutex>
#include <map>
#include <thread>
#include <condition_variable>
#include <atomic>
#include "task.h"
#include "node.h"

struct SchedulerStats {
    size_t total_tasks = 0;
    size_t speculative_tasks = 0;
    size_t stragglers_detected = 0;
    size_t tasks_completed = 0;
    std::map<int, double> task_durations;
};

class Scheduler {
private:
    std::vector<std::shared_ptr<Node>> nodes;
    std::vector<std::shared_ptr<Task>> tasks;
    std::mutex mtx;
    SchedulerStats stats;
    int speculative_limit;
    double straggler_percentile;
    std::atomic<bool> running;
    std::condition_variable cv;
    std::thread scheduler_thread;

public:
    Scheduler(int speculative_limit = 2, double straggler_percentile = 0.2);
    void addNode(std::shared_ptr<Node> node);
    void addTask(std::shared_ptr<Task> task);
    void start();
    void join();
    SchedulerStats getStats() const;
    void recordCompletion(std::shared_ptr<Task> task, double duration);

private:
    void schedulerLoop();
    void assignTasks();
    void monitorSpeculation();
};

#endif // SCHEDULER_H