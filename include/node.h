#ifndef NODE_H
#define NODE_H

#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include "task.h"

class Scheduler;

class Node {
public:
    int id;
    double speed_factor;
    std::atomic<bool> busy;
    std::shared_ptr<Scheduler> scheduler;

    Node(int id, double speed, std::shared_ptr<Scheduler> scheduler);
    void run(std::shared_ptr<Task> task);
};

#endif // NODE_H