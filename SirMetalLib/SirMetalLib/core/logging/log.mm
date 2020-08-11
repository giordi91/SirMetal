#include "log.h"
#include <iostream>
#include "spdlog/sinks/stdout_color_sinks.h"
#import "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"

namespace SirMetal {

    std::shared_ptr<spdlog::logger> Log::s_coreLogger;
    std::shared_ptr<spdlog::logger> Log::s_asyncFileLogger;
    BufferedSink_mt *Log::bufferedSink;

    void Log::init(const std::string &path) {
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

        bufferedSink = new BufferedSink_mt;
        bufferedSink->set_pattern("[%l] %v%$");
        sinks.push_back(std::shared_ptr<BufferedSink_mt>(bufferedSink));

        sinks[0]->set_pattern("%^[%T] %n [%l]: %v%$");
        s_coreLogger = std::make_shared<spdlog::logger>("SirEngine", begin(sinks), end(sinks));

        //s_coreLogger = spdlog::stdout_color_mt("SirEngine");
        //s_coreLogger = spdlog::basic_logger_mt("basic_logger", "/Users/marcogiordano/WORK_IN_PROGRESS/SirMetalProject/basic.txt");;
        s_coreLogger->set_level(spdlog::level::trace);

        s_asyncFileLogger = spdlog::basic_logger_mt<spdlog::async_factory>("async_file_logger", path.c_str());
        s_asyncFileLogger->set_pattern("%^[%T] %n: %v%$");
        spdlog::flush_every(std::chrono::seconds(5));
    }

    void Log::free() {

        spdlog::drop(s_coreLogger->name());
        s_coreLogger.reset();
        s_coreLogger = nullptr;

        spdlog::drop(s_asyncFileLogger->name());
        s_asyncFileLogger.reset();
        s_asyncFileLogger= nullptr;
    }
}
