#include "scheduler.h"
#include "utils.h"
#include <iostream>
#include <algorithm>

Scheduler::Scheduler(int speculative_limit, double straggler_percentile)
    : speculative_limit(speculative_limit), straggler_percentile(straggler_percentile), running(false) {}

void Scheduler::addNode(std::shared_ptr<Node> node) {
    std::lock_guard<std::mutex> lock(mtx);
    nodes.push_back(node);
    std::cout << "[" << currentTimestamp() << "] Added node " << node->id << "\n";
}

void Scheduler::addTask(std::shared_ptr<Task> task) {
    std::lock_guard<std::mutex> lock(mtx);
    tasks.push_back(task);
    stats.total_tasks++;
    std::cout << "[" << currentTimestamp() << "] Added task " << task->id << "\n";
}

void Scheduler::start() {
    running = true;
    scheduler_thread = std::thread(&Scheduler::schedulerLoop, this);
}

void Scheduler::join() {
    scheduler_thread.join();
}

void Scheduler::schedulerLoop() {
    while (running) {
        assignTasks();
        monitorSpeculation();
        {
            std::unique_lock<std::mutex> lock(mtx);
            if (stats.tasks_completed >= stats.total_tasks) {
                running = false;
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    std::cout << "[" << currentTimestamp() << "] Scheduler finished.\n";
}

void Scheduler::assignTasks() {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& node : nodes) {
        if (!node->busy) {
            auto it = std::find_if(tasks.begin(), tasks.end(), [](auto& t) {
                return !t->completed && !t->in_progress;
            });
            if (it != tasks.end()) {
                (*it)->node_speed_factor = node->speed_factor;
                (*it)->in_progress = true;
                node->run(*it);
            }
        }
    }
}

void Scheduler::monitorSpeculation() {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<std::shared_ptr<Task>> candidates;
    for (auto& t : tasks) {
        if (!t->completed && !t->is_speculative && t->in_progress)
            candidates.push_back(t);
    }
    if (candidates.empty()) return;

    std::sort(candidates.begin(), candidates.end(), [](auto& a, auto& b) {
        return a->getEstimatedTimeToEnd() > b->getEstimatedTimeToEnd();
    });

    size_t limit = std::min<size_t>(speculative_limit, candidates.size() * straggler_percentile);
    for (size_t i = 0; i < limit; ++i) {
        auto original = candidates[i];
        auto spec = std::make_shared<Task>(original->id + 10000, original->data);
        spec->is_speculative = true;
        spec->in_progress = false;
        addTask(spec);
        stats.speculative_tasks++;
        stats.stragglers_detected++;
    }
}

void Scheduler::recordCompletion(std::shared_ptr<Task> task, double duration) {
    std::lock_guard<std::mutex> lock(mtx);
    stats.tasks_completed++;
    stats.task_durations[task->id] = duration;
    std::cout << "[" << currentTimestamp() << "] Task " << task->id << " completed in " << duration << "s\n";
}

SchedulerStats Scheduler::getStats() const {
    return stats;
}