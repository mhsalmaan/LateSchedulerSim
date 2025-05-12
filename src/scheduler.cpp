#include "scheduler.h"
#include "utils.h"
#include <algorithm>
#include <iostream>
#include <set>

Scheduler::Scheduler(int speculative_limit, double straggler_percentile)
    : speculative_limit(speculative_limit), straggler_percentile(straggler_percentile), running(false) {
    stats.straggler_percentile = straggler_percentile;
}

void Scheduler::addNode(std::shared_ptr<Node> node) {
    std::lock_guard<std::mutex> lock(mtx);
    if (node) {
        nodes.push_back(node);
    }
}

void Scheduler::addTask(std::shared_ptr<Task> task) {
    std::lock_guard<std::mutex> lock(mtx);
    if (task) {
        tasks.push_back(task);
        stats.total_tasks++;
    }
}

void Scheduler::start() {
    running = true;
    scheduler_thread = std::thread(&Scheduler::schedulerLoop, this);
}

void Scheduler::join() {
    if (scheduler_thread.joinable()) {
        scheduler_thread.join();
    }
}

void Scheduler::schedulerLoop() {
    while (running) {
        assignTasks();
        monitorSpeculation();
        {
            std::unique_lock<std::mutex> lock(mtx);
            if (stats.tasks_completed >= stats.total_tasks && stats.total_tasks > 0) {
                running = false;
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void Scheduler::assignTasks() {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& node : nodes) {
        if (node && !node->busy) {
            auto it = std::find_if(tasks.begin(), tasks.end(), [](auto& t) {
                return t && !t->completed && !t->in_progress;
            });
            if (it != tasks.end()) {
                (*it)->node_speed_factor = node->speed_factor;
                (*it)->in_progress = true;
                node->run(*it);
                std::cout << "Assigned task " << (*it)->id << " to node " << node->id << "\n";
            }
        }
    }
}

void Scheduler::monitorSpeculation() {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<std::shared_ptr<Task>> candidates;
    std::set<int> speculative_original_ids; // Track tasks with speculative versions

    // Collect candidates and check for existing speculative tasks
    for (auto& t : tasks) {
        if (t && !t->completed && !t->is_speculative && t->in_progress && t->getProgress() < 0.9) {
            candidates.push_back(t);
        }
        if (t && t->is_speculative) {
            speculative_original_ids.insert(t->id - 10000); // Original task ID
        }
    }
    if (candidates.empty()) return;

    std::sort(candidates.begin(), candidates.end(), [](auto& a, auto& b) {
        return a->getEstimatedTimeToEnd() > b->getEstimatedTimeToEnd();
    });

    size_t limit = std::min<size_t>(speculative_limit, std::max<size_t>(1, candidates.size() * straggler_percentile));
    std::vector<std::shared_ptr<Task>> new_speculative_tasks;
    for (size_t i = 0; i < limit && i < candidates.size(); ++i) {
        auto original = candidates[i];
        if (original && original->data.size() < 1024 * 1024 && // Limit data size to 1MB
            speculative_original_ids.find(original->id) == speculative_original_ids.end()) { // No speculative task yet
            try {
                auto spec = std::make_shared<Task>(original->id + 10000, original->data);
                if (spec) {
                    spec->is_speculative = true;
                    spec->in_progress = false;
                    new_speculative_tasks.push_back(spec);
                    stats.speculative_tasks++;
                    stats.stragglers_detected++;
                    speculative_original_ids.insert(original->id); // Mark as having a speculative task
                    std::cout << "Created speculative task " << spec->id << " for task " << original->id << "\n";
                }
            } catch (const std::exception& e) {
                std::cout << "Failed to create speculative task: " << e.what() << "\n";
            }
        }
    }

    tasks.insert(tasks.end(), new_speculative_tasks.begin(), new_speculative_tasks.end());
}

void Scheduler::recordCompletion(std::shared_ptr<Task> task, double duration) {
    std::lock_guard<std::mutex> lock(mtx);
    if (task && duration >= 0) {
        std::cout << "Completed task " << task->id << " in " << duration << "s\n";
        stats.tasks_completed++;
        stats.task_durations[task->id] = duration;
    }
}

SchedulerStats Scheduler::getStatsRaw() {
    std::lock_guard<std::mutex> lock(mtx);
    return stats;
}