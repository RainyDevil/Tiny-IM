#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
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

    // 获取当前的 Unix 时间戳
    static time_t getCurrentUnixTimestamp() {
        return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    }

    // 将 std::tm 转换为指定格式的日期字符串
    static std::string formatDate(const std::tm& tm, const std::string& format = "%Y-%m-%d %H:%M:%S") {
        std::ostringstream oss;
        oss << std::put_time(&tm, format.c_str());
        return oss.str();
    }

    // 将日期字符串转换为 Unix 时间戳
    static time_t dateToUnixTimestamp(const std::string& date, const std::string& format = "%Y-%m-%d %H:%M:%S") {
        std::tm tm = {};
        std::istringstream ss(date);
        ss >> std::get_time(&tm, format.c_str());
        if (ss.fail()) {
            throw std::runtime_error("Failed to parse date string");
        }
        return mktime(&tm);
    }
};

#endif // UTILS_H
