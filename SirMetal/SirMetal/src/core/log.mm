#include "log.h"
#include <iostream>
#include "spdlog/sinks/stdout_color_sinks.h"
#import "basic_file_sink.h"

namespace SirMetal {

    std::shared_ptr<spdlog::logger> Log::s_coreLogger;
    std::shared_ptr<spdlog::logger> Log::s_clientLogger;
    //auto file_logger = spdlog::basic_logger_mt("basic_logger", "logs/basic.txt");

    void Log::init()
    {
        /*
        std::vector<spdlog::sink_ptr> sinks;
        auto async_file = spdlog::basic_logger_mt<spdlog::async_factory>("async_file_logger", "logs/async_log.txt");
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
        sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_st>("logfile", 23, 59));
        auto combined_logger = std::make_shared<spdlog::logger>("name", begin(sinks), end(sinks));
//register it if you need to access it globally
        spdlog::register_logger(combined_logger);
         */

        spdlog::set_pattern("%^[%T] %n: %v%$");
        s_coreLogger = spdlog::stdout_color_mt("SirEngine");
        //s_coreLogger = spdlog::basic_logger_mt("basic_logger", "/Users/marcogiordano/WORK_IN_PROGRESS/SirMetalProject/basic.txt");;
        s_coreLogger->set_level(spdlog::level::trace);
        s_clientLogger= spdlog::stdout_color_mt("APP");
        s_clientLogger->set_level(spdlog::level::trace);
    }

    void Log::free()
    {

        spdlog::drop(s_coreLogger->name());
        spdlog::drop(s_clientLogger->name());
        s_coreLogger.reset();
        s_clientLogger.reset();
        s_coreLogger = nullptr;
        s_clientLogger= nullptr;
    }
}