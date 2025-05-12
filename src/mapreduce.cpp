#include "mapreduce.h"
#include "task.h"
#include "utils.h"
#include <fstream>
#include <sstream>

MapReduce::MapReduce(const std::string& file, int num_tasks, std::shared_ptr<Scheduler> scheduler)
    : input_file(file), num_tasks(num_tasks), scheduler(scheduler) {}

void MapReduce::splitInput() {
    std::ifstream file(input_file);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    size_t chunk_size = content.size() / num_tasks;
    for (int i = 0; i < num_tasks; ++i) {
        size_t start = i * chunk_size;
        chunks.push_back(content.substr(start, chunk_size));
    }
}

void MapReduce::createTasks() {
    for (int i = 0; i < num_tasks; ++i) {
        auto task = std::make_shared<Task>(i, chunks[i]);
        scheduler->addTask(task);
    }
}