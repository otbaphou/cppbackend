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
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const
    {
        const std::chrono::time_point now{ std::chrono::system_clock::now() };
        const std::chrono::year_month_day ymd{ std::chrono::floor<std::chrono::days>(now) };

        std::stringstream date; 
        date << ymd;

        return date.str();
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
