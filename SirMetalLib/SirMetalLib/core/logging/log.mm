#include "log.h"
#include <iostream>
#include "spdlog/sinks/stdout_color_sinks.h"
#import "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"

namespace SirMetal {

    std::shared_ptr<spdlog::logger> Log::s_coreLogger;

    void Log::init(const std::string &path) {
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

        sinks[0]->set_pattern("%^[%T] %n [%l]: %v%$");
        s_coreLogger = std::make_shared<spdlog::logger>("SirEngine", begin(sinks), end(sinks));
        s_coreLogger->set_level(spdlog::level::trace);
    }

    void Log::free() {

        spdlog::drop(s_coreLogger->name());
        s_coreLogger.reset();
        s_coreLogger = nullptr;
    }
}
