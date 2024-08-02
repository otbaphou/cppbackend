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
#include <syncstream>

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
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);

        char mbstr[100];
        std::strftime(mbstr, 100, "%Y_%m_%d", std::localtime(&t_c));

        std::string time(mbstr);

        return time;
    }

    Logger() = default;
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance()
    {
        static Logger obj;
        return obj;
    }

    void SetPath()
    {
        std::string raw_path = "/var/log/";
        raw_path += current_date;
        raw_path += ".log";

        std::filesystem::path tmpath{ raw_path };
        current_path = std::move(tmpath);

        log_file_.close();

        log_file_ = std::ofstream{current_path, std::ios::app };
        log_file_.open(current_path);
    }

    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... args)
    {
        std::string current_timestamp = GetFileTimeStamp();

        if (current_date != current_timestamp || !log_file_.is_open())
        {
            current_date = std::move(current_timestamp);
            SetPath();
        }

        std::osyncstream stream {log_file_};

        stream << GetTimeStamp() << ": "sv;
        ((stream << args), ...);
        stream << std::endl;
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

    std::string current_date = "";
    std::filesystem::path current_path = "";
    std::ofstream log_file_;

    std::optional<std::chrono::system_clock::time_point> manual_ts_;
};
