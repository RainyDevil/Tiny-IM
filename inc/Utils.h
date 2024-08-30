#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <string>
#include <stdexcept>

class Utils {
public:
    // 将 Unix 时间戳转换为 "YYYY-MM-DD HH:MM:SS" 格式的日期字符串
    static std::string unixTimestampToDate(time_t unixTimestamp) {
        std::chrono::system_clock::time_point timePoint = std::chrono::system_clock::from_time_t(unixTimestamp);
        std::time_t time = std::chrono::system_clock::to_time_t(timePoint);

        std::tm tm;
        if (localtime_r(&time, &tm) == nullptr) {
            throw std::runtime_error("Failed to convert time to tm structure");
        }
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    static time_t getCurrentUnixTimestamp() {
        return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    }

    static std::string formatDate(const std::tm& tm, const std::string& format = "%Y-%m-%d %H:%M:%S") {
        std::ostringstream oss;
        oss << std::put_time(&tm, format.c_str());
        return oss.str();
    }
    static time_t dateToUnixTimestamp(const std::string& date, const std::string& format = "%Y-%m-%d %H:%M:%S") {
        std::tm tm = {};
        std::istringstream ss(date);
        ss >> std::get_time(&tm, format.c_str());
        if (ss.fail()) {
            throw std::runtime_error("Failed to parse date string");
        }
        return mktime(&tm);
    }

    static std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string item;
        while (std::getline(ss, item, delimiter)) {
            tokens.push_back(item);
        }
        return tokens;
    }

    static void printProcessStats(int pid) {
        std::ifstream statFile("/proc/" + std::to_string(pid) + "/stat");
        if (!statFile.is_open()) {
            std::cerr << "Error opening file: /proc/" << pid << "/stat" << std::endl;
            return;
        }

        std::string line;
        std::getline(statFile, line);
        statFile.close();
        std::vector<std::string> tokens = split(line, ' ');

        if (tokens.size() < 22) {
            std::cerr << "Unexpected format in /proc/" << pid << "/stat" << std::endl;
            return;
        }
        std::string processName = tokens[1];
        long utime = std::stol(tokens[13]);  // User mode jiffies
        long stime = std::stol(tokens[14]);  // Kernel mode jiffies
        long vsize = std::stol(tokens[22]);  // Virtual memory size in bytes
        long rss = std::stol(tokens[23]);    // Resident Set Size (pages)

        std::cout << "Process ID: " << pid << std::endl;
        std::cout << "Process Name: " << processName << std::endl;
        std::cout << "User Time (jiffies): " << utime << std::endl;
        std::cout << "System Time (jiffies): " << stime << std::endl;
        std::cout << "Virtual Memory Size (bytes): " << vsize << std::endl;
        std::cout << "Resident Set Size (pages): " << rss << std::endl;
    }
};

#endif // UTILS_H
