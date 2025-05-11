#include "node.h"
#include "scheduler.h"
#include "utils.h"
#include <iostream>

Node::Node(int id, double speed, std::shared_ptr<Scheduler> scheduler)
    : id(id), speed_factor(speed), busy(false), scheduler(scheduler) {}

void Node::run(std::shared_ptr<Task> task) {
    busy = true;
    std::cout << "[" << currentTimestamp() << "] Node " << id << " assigned task " << task->id << "\n";
    std::thread([this, task]() {
        task->markStarted();
        simulateWork(1.0 / speed_factor);
        task->markCompleted();
        scheduler->recordCompletion(task, task->getDuration());
        busy = false;
    }).detach();
}
