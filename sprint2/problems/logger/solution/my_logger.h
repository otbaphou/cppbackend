#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <filesystem>
#include <mutex>
#include <thread>
#include <format>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger
{
    std::chrono::system_clock::time_point GetTime() const
    {
        if (manual_ts_)
        {
            return *manual_ts_;
        }

        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const
    {
        return std::format("{:%Y-%m-%d}", std::localtime);
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const
    {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::format("{:%Y-%m-%d}", t_c);
    }

    Logger() = default;
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance()
    {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... args)
    {
        ///var/log/
        std::string raw_path = "C:/Users/0tba/Desktop/";
        raw_path += GetFileTimeStamp();
        raw_path += ".log";

        std::filesystem::path log_path{ raw_path };
        std::cout << log_path.string() << '\n';
        std::ofstream log_file_{ log_path, std::ios::app };

        log_file_ << GetTimeStamp() << ": "sv;
        ((log_file_ << args), ...);
        log_file_ << std::endl;
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть 
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts)
    {
        //TODO: Make sure it doesn't break the conditions above
        manual_ts_.emplace(ts);
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
};
