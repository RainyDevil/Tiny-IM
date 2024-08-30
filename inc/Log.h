#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <string>
#include <memory>
#include <utility>
#include <algorithm>  // 用于 std::transform

// 日志级别
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

// 日志条目结构
struct LogEntry {
    LogLevel level;
    std::string message;
};

// 日志类
class Logger {
public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // 获取单例实例
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    // 使用字符串设置日志级别
    void setLogLevel(const std::string& level) {
        std::string level_lower = level;
        std::transform(level_lower.begin(), level_lower.end(), level_lower.begin(), ::tolower);

        if (level_lower == "debug") {
            log_level = LogLevel::DEBUG;
        } else if (level_lower == "info") {
            log_level = LogLevel::INFO;
        } else if (level_lower == "warning") {
            log_level = LogLevel::WARNING;
        } else if (level_lower == "error") {
            log_level = LogLevel::ERROR;
        } else {
            std::cerr << "Invalid log level: " << level << ". Using default level: DEBUG." << std::endl;
            log_level = LogLevel::DEBUG;
        }
    }

    // 记录日志（带有函数信息）
    template<typename... Args>
    void log(LogLevel level, const char* file, int line, const char* func, const std::string& format, Args&&... args) {
        if (level < log_level) {
            return;  // 如果日志级别低于全局设置的级别，则忽略
        }

        std::string message = formatString(format, std::forward<Args>(args)...);
        std::ostringstream oss;
        oss << "[" << file << ":" << line << " - " << func << "] " << message;
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            log_queue.push({level, oss.str()});
        }
        cv.notify_one();
    }

private:
    std::atomic<bool> running;
    std::thread log_thread;
    std::queue<LogEntry> log_queue;
    std::mutex queue_mutex;
    std::condition_variable cv;
    LogLevel log_level = LogLevel::DEBUG;  // 默认日志级别为 DEBUG

    Logger() : running(true), log_thread(&Logger::processEntries, this) {
        displayBanner();
    }

    ~Logger() {
        stop();
    }

    // 处理日志条目
    void processEntries() {
        while (running) {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, [this] { return !log_queue.empty() || !running; });

            while (!log_queue.empty()) {
                LogEntry entry = log_queue.front();
                log_queue.pop();
                lock.unlock();

                printLog(entry.level, entry.message);

                lock.lock();
            }
        }
    }

    // 停止日志线程
    void stop() {
        running = false;
        cv.notify_all();
        if (log_thread.joinable()) {
            log_thread.join();
        }
    }

    // 打印日志消息到控制台
    void printLog(LogLevel level, const std::string& message) {
        std::string level_str;
        std::string color_code;

        switch (level) {
            case LogLevel::DEBUG:
                level_str = "DEBUG";
                color_code = "\033[1;34m";  // 蓝色
                break;
            case LogLevel::INFO:
                level_str = "INFO";
                color_code = "\033[1;32m";  // 绿色
                break;
            case LogLevel::WARNING:
                level_str = "WARNING";
                color_code = "\033[1;33m";  // 黄色
                break;
            case LogLevel::ERROR:
                level_str = "ERROR";
                color_code = "\033[1;31m";  // 红色
                break;
        }

        // 打印带颜色的日志信息
        std::cout << color_code << "[" << level_str << "] " << message << "\033[0m" << std::endl;
    }

    // 显示启动字符图案
    void displayBanner() {
        std::cout << "***********************" << std::endl;
        std::cout << "*   Welcome to Tiny-IM    *" << std::endl;
        std::cout << "***********************" << std::endl;
    }

    // 格式化字符串
    template<typename... Args>
    std::string formatString(const std::string& format, Args&&... args) {
        std::ostringstream oss;
        formatImpl(oss, format, std::forward<Args>(args)...);
        return oss.str();
    }

    // 递归替换占位符实现
    void formatImpl(std::ostringstream& oss, const std::string& format) {
        oss << format;
    }

    template<typename T, typename... Args>
    void formatImpl(std::ostringstream& oss, const std::string& format, T&& arg, Args&&... args) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            oss << format.substr(0, pos) << std::forward<T>(arg);
            formatImpl(oss, format.substr(pos + 2), std::forward<Args>(args)...);
        } else {
            oss << format;
        }
    }
};

// 宏定义简化日志记录，添加文件、行号和函数信息
#define LOG_DEBUG(format, ...) Logger::getInstance().log(LogLevel::DEBUG, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) Logger::getInstance().log(LogLevel::INFO, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...) Logger::getInstance().log(LogLevel::WARNING, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) Logger::getInstance().log(LogLevel::ERROR, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#endif // LOG_H
