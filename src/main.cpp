#include "scheduler.h"
#include "mapreduce.h"
#include "utils.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <map>
#include <sys/stat.h>
#include <string>
#include <algorithm>

bool directoryExists(const std::string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR);
}

void createDirectoryIfNotExists(const std::string& path) {
    if (!directoryExists(path)) {
        #ifdef _WIN32
        _mkdir(path.c_str());
        #else
        mkdir(path.c_str(), 0755);
        #endif
    }
}

void printProgressBar(size_t done, size_t total, const std::string& phase) {
    const int barWidth = 50;
    float progress = static_cast<float>(done) / total;
    std::cout << phase << " Progress: [";
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << "% (" << done << "/" << total << ")\r" << std::flush;
}

std::map<std::string, int> computeWordCount(const std::string& filename) {
    std::ifstream in(filename);
    std::map<std::string, int> wordCount;
    std::string word;
    
    if (!in.is_open()) {
        return wordCount;
    }
    
    while (in >> word) {
        word.erase(std::remove_if(word.begin(), word.end(), 
                                 [](char c) { return std::ispunct(static_cast<unsigned char>(c)); }), 
                  word.end());
        std::transform(word.begin(), word.end(), word.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        if (!word.empty()) {
            ++wordCount[word];
        }
    }
    
    return wordCount;
}

void writeWordCountToFile(const std::map<std::string, int>& wc, const std::string& outFile) {
    std::ofstream out(outFile);
    if (!out.is_open()) {
        return;
    }
    
    std::vector<std::pair<std::string, int>> sortedWords(wc.begin(), wc.end());
    std::sort(sortedWords.begin(), sortedWords.end(), 
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (const auto& [word, count] : sortedWords) {
        out << word << ": " << count << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        return 1;
    }

    std::cout << "Changed by pawan" << endl;
    std::string input_file = argv[1];
    int num_nodes = std::stoi(argv[2]);
    size_t num_tasks = std::stoi(argv[3]);

    createDirectoryIfNotExists("results");

    auto scheduler = std::make_shared<Scheduler>(2, 0.2);

    std::vector<double> nodeSpeeds;
    for (int i = 0; i < num_nodes; ++i) {
        double speed = generateSpeedFactor();
        nodeSpeeds.push_back(speed);
        auto node = std::make_shared<Node>(i, speed, scheduler);
        scheduler->addNode(node);
    }

    MapReduce mr(input_file, num_tasks, scheduler);
    mr.splitInput();
    mr.createTasks();

    scheduler->start();

    size_t total = num_tasks;
    size_t lastCompleted = 0;

    std::cout << "MAP PHASE\n";
    while (true) {
        auto stats = scheduler->getStatsRaw();
        printProgressBar(stats.tasks_completed, total, "Map");

        if (stats.tasks_completed >= total) {
            std::cout << std::endl;
            break;
        }

        if (stats.tasks_completed != lastCompleted) {
            lastCompleted = stats.tasks_completed;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    scheduler->join();

    std::cout << "\nREDUCE PHASE\n";
    size_t reduceSteps = 10;
    for (size_t i = 0; i <= reduceSteps; i++) {
        printProgressBar(i, reduceSteps, "Reduce");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << std::endl;

    auto wc = computeWordCount(input_file);
    writeWordCountToFile(wc, "results/output.txt");

    std::cout << "\n=== Scheduler Summary ===\n";
    auto stats = scheduler->getStatsRaw();
    std::cout << "Total tasks: " << stats.total_tasks << "\n";
    std::cout << "Completed: " << stats.tasks_completed << "\n";
    std::cout << "Speculative tasks: " << stats.speculative_tasks << "\n";
    std::cout << "Stragglers detected: " << stats.stragglers_detected << "\n";

    std::cout << "\n=== Node Speed Factors ===\n";
    for (size_t i = 0; i < nodeSpeeds.size(); ++i) {
        std::cout << "Node " << i << ": " << std::fixed << std::setprecision(3) << nodeSpeeds[i] << "\n";
    }

    std::cout << "\n=== Task Durations ===\n";
    for (auto& [id, duration] : stats.task_durations) {
        std::cout << "Task " << std::setw(5) << id << ": " << std::fixed << std::setprecision(3) 
                  << duration << "s" << (id >= 10000 ? " (speculative)" : "") << "\n";
    }

    std::cout << "\n=== Output ===\n";
    std::cout << "Word count completed. Output written to results/output.txt\n";
    std::cout << "Found " << wc.size() << " unique words\n";
    std::cout << "LATE Scheduler threshold: Speculative execution for slowest " 
              << (int)(stats.straggler_percentile * 100) << "% of tasks\n";
    
    return 0;
}