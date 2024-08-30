#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

class Config {
public:
    // ��ȡ Config ��ĵ���ʵ��
    static Config& getInstance() {
        static Config instance;
        return instance;
    }
    // ��ȡ������
    int getPort() const { return port_; }
    std::string getDbName() const { return dbName_; }
    std::string getHost() const { return host_; }
    std::string getLogLevel() const { return logLevel_; }
    int getConnectionSize() const { return connectionSize_; }
private:
    // ���캯��˽�л�
    Config() {
        loadConfig("config.json");
    }
    // ��ֹ��������͸�ֵ����
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    void loadConfig(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << filename << std::endl;
            return;
        }

        try {
            nlohmann::json configJson;
            file >> configJson;

            port_ = configJson.at("port").get<int>();
            dbName_ = configJson.at("db_name").get<std::string>();
            host_ = configJson.at("host").get<std::string>();
            logLevel_ = configJson.at("log_level").get<std::string>();
            connectionSize_ = configJson.at("connection_size").get<int>();

        } catch (const std::exception& e) {
            std::cerr << "Error reading config file: " << e.what() << std::endl;
        }
    }

    int port_;
    std::string dbName_;
    std::string host_;
    std::string logLevel_;
    int connectionSize_;
};
