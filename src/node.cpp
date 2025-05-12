#include "node.h"
#include "scheduler.h"
#include "utils.h"

Node::Node(int id, double speed, std::shared_ptr<Scheduler> scheduler)
    : id(id), speed_factor(speed), busy(false), scheduler(scheduler) {}

void Node::run(std::shared_ptr<Task> task) {
    if (!task || !scheduler) return;
    busy = true;
    std::thread([this, task]() {
        task->markStarted();
        simulateWork(1.0 / speed_factor);
        task->markCompleted();
        scheduler->recordCompletion(task, task->getDuration());
        busy = false;
    }).detach(); // Revert to detach for parallelism
}