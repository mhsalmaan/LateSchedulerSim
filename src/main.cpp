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

// Function to check if directory exists
bool directoryExists(const std::string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR);
}

// Create directory if it doesn't exist
void createDirectoryIfNotExists(const std::string& path) {
    if (!directoryExists(path)) {
        #ifdef _WIN32
        _mkdir(path.c_str());
        #else
        mkdir(path.c_str(), 0755);
        #endif
        std::cout << "[" << currentTimestamp() << "] Created directory: " << path << "\n";
    }
}

void printProgressBar(int done, int total, const std::string& phase) {
    const int barWidth = 40;
    float progress = static_cast<float>(done) / total;
    std::cout << "[" << currentTimestamp() << "] " << phase << " Progress: [";
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << "%\r" << std::flush;
}

std::map<std::string, int> computeWordCount(const std::string& filename) {
    std::ifstream in(filename);
    std::map<std::string, int> wordCount;
    std::string word;
    
    if (!in.is_open()) {
        std::cerr << "[ERROR] Cannot open file: " << filename << "\n";
        return wordCount;
    }
    
    // Simple word counting logic - split by whitespace
    while (in >> word) {
        // Remove punctuation from word
        word.erase(std::remove_if(word.begin(), word.end(), 
                                 [](char c) { return std::ispunct(static_cast<unsigned char>(c)); }), 
                  word.end());
        
        // Convert to lowercase
        std::transform(word.begin(), word.end(), word.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        
        if (!word.empty()) {
            ++wordCount[word];
        }
    }
    
    std::cout << "[" << currentTimestamp() << "] Counted " << wordCount.size() << " unique words\n";
    return wordCount;
}

void writeWordCountToFile(const std::map<std::string, int>& wc, const std::string& outFile) {
    std::ofstream out(outFile);
    if (!out.is_open()) {
        std::cerr << "[ERROR] Cannot open output file: " << outFile << "\n";
        return;
    }
    
    // Sort words by frequency (highest count first)
    std::vector<std::pair<std::string, int>> sortedWords(wc.begin(), wc.end());
    std::sort(sortedWords.begin(), sortedWords.end(), 
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Write to file
    for (const auto& [word, count] : sortedWords) {
        out << word << ": " << count << "\n";
    }
    
    std::cout << "[" << currentTimestamp() << "] Word count successfully written to " << outFile << "\n";
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <num_nodes> <num_tasks>\n";
        return 1;
    }

    std::string input_file = argv[1];
    int num_nodes = std::stoi(argv[2]);
    int num_tasks = std::stoi(argv[3]);

    // Create results directory if it doesn't exist
    createDirectoryIfNotExists("results");

    std::cout << "[" << currentTimestamp() << "] Starting LATE Scheduler...\n";
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

    int total = num_tasks;
    int lastCompleted = 0;

    while (true) {
        auto stats = scheduler->getStats();
        printProgressBar(stats.tasks_completed, total, "Map Phase");

        if (stats.tasks_completed >= total) break;

        if (stats.tasks_completed != lastCompleted) {
            std::cout << "\n[" << currentTimestamp() << "] Completed Tasks: " << stats.tasks_completed << "/" << total << "\n";
            std::cout << "[Info] Speculative: " << stats.speculative_tasks
                      << ", Stragglers: " << stats.stragglers_detected << "\n";
            for (auto& [id, duration] : stats.task_durations) {
                std::cout << "  - Task " << id << " duration: " << std::fixed << std::setprecision(3) << duration << "s"
                          << (id >= 10000 ? " (speculative)" : "") << "\n";
            }
            lastCompleted = stats.tasks_completed;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    scheduler->join();

    std::cout << "\n\n=== Scheduler Summary ===\n";
    auto stats = scheduler->getStats();
    std::cout << "Total tasks: " << stats.total_tasks << "\n";
    std::cout << "Completed: " << stats.tasks_completed << "\n";
    std::cout << "Speculative: " << stats.speculative_tasks << "\n";
    std::cout << "Stragglers detected: " << stats.stragglers_detected << "\n";

    std::cout << "\n=== Node Speed Factors ===\n";
    for (size_t i = 0; i < nodeSpeeds.size(); ++i) {
        std::cout << "Node " << i << ": " << nodeSpeeds[i] << "\n";
    }

    std::cout << "\n=== Task Durations ===\n";
    for (auto& [id, duration] : stats.task_durations) {
        std::cout << "Task " << id << ": " << std::fixed << std::setprecision(3) << duration << "s"
                  << (id >= 10000 ? " (speculative)" : "") << "\n";
    }

    std::cout << "\n[" << currentTimestamp() << "] Simulating reduce phase (output generation)...\n";
    printProgressBar(total, total, "Reduce Phase");
    std::cout << "\n";

    auto wc = computeWordCount(input_file);
    writeWordCountToFile(wc, "results/output.txt");
    
    std::cout << "[" << currentTimestamp() << "] Word count written to results/output.txt\n";
    std::cout << "[Threshold] Speculative execution triggers for slowest 20% of running tasks.\n";
    return 0;
}