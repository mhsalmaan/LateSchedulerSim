#ifndef MAPREDUCE_H
#define MAPREDUCE_H

#include <string>
#include <vector>
#include <memory>
#include "scheduler.h"

class MapReduce {
public:
    MapReduce(const std::string& file, int num_tasks, std::shared_ptr<Scheduler> scheduler);
    void splitInput();
    void createTasks();

private:
    std::string input_file;
    int num_tasks;
    std::vector<std::string> chunks;
    std::shared_ptr<Scheduler> scheduler;
};

#endif // MAPREDUCE_H
