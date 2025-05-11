#include "utils.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>

std::string currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

double generateSpeedFactor() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.5, 1.5);
    return dis(gen);
}

void simulateWork(double duration) {
    std::this_thread::sleep_for(std::chrono::milliseconds(int(duration * 1000)));
}
