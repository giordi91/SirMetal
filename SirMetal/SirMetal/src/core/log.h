#pragma once

#include "../vendors/spdlog/spdlog.h"
#include "../vendors/spdlog/fmt/ostr.h"
#import "bufferedLog.h"

namespace SirMetal {

    class Log {
    public:
        static void init(const std::string&path);
        static void free();

        inline static std::shared_ptr<spdlog::logger> &
        getCoreLogger() {
            return s_coreLogger;
        }
        inline static std::shared_ptr<spdlog::logger> &
        getAsyncFileLogger() {
            return s_asyncFileLogger;
        }

        static const std::string* getBuffer(){return bufferedSink->getBuffer();}

    private:
        static std::shared_ptr<spdlog::logger> s_coreLogger;
        static std::shared_ptr<spdlog::logger> s_asyncFileLogger;
        static BufferedSink_mt *bufferedSink;
    };
} // namespace SirMetal

#define SIR_CORE_TRACE(...) ::SirMetal::Log::getCoreLogger()->trace(__VA_ARGS__)
#define SIR_CORE_INFO(...) ::SirMetal::Log::getCoreLogger()->info(__VA_ARGS__);::SirMetal::Log::getAsyncFileLogger()->info(__VA_ARGS__)
#define SIR_CORE_WARN(...) ::SirMetal::Log::getCoreLogger()->warn(__VA_ARGS__);::SirMetal::Log::getAsyncFileLogger()->info(__VA_ARGS__)
#define SIR_CORE_ERROR(...) ::SirMetal::Log::getCoreLogger()->error(__VA_ARGS__);::SirMetal::Log::getAsyncFileLogger()->info(__VA_ARGS__)
#define SIR_CORE_FATAL(...) ::SirMetal::Log::getCoreLogger()->critical(__VA_ARGS__);::SirMetal::Log::getAsyncFileLogger()->info(__VA_ARGS__)

