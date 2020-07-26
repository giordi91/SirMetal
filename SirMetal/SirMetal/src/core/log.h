#pragma once

#include "../vendors/spdlog/spdlog.h"
#include "../vendors/spdlog/fmt/ostr.h"

namespace SirMetal {

    class Log {
    public:
        static void init();

        static void free();

        inline static std::shared_ptr<spdlog::logger> &
        getCoreLogger() {
            return s_coreLogger;
        }

        inline static std::shared_ptr<spdlog::logger> &
        getClientLogger() {
            return s_clientLogger;
        }

    private:
        static std::shared_ptr<spdlog::logger> s_coreLogger;
        static std::shared_ptr<spdlog::logger> s_clientLogger;
    };
} // namespace SirMetal

#define SIR_CORE_TRACE(...) ::SirMetal::Log::getCoreLogger()->trace(__VA_ARGS__)
#define SIR_CORE_INFO(...) ::SirMetal::Log::getCoreLogger()->info(__VA_ARGS__)
#define SIR_CORE_WARN(...) ::SirMetal::Log::getCoreLogger()->warn(__VA_ARGS__)
#define SIR_CORE_ERROR(...) ::SirMetal::Log::getCoreLogger()->error(__VA_ARGS__)
#define SIR_CORE_FATAL(...) ::SirMetal::Log::getCoreLogger()->fatal(__VA_ARGS__)

#define SIR_TRACE(...) ::SirMetal::Log::getClientLogger()->trace(__VA_ARGS__)
#define SIR_INFO(...) ::SirMetal::Log::getClientLogger()->info(__VA_ARGS__)
#define SIR_WARN(...) ::SirMetal::Log::getClientLogger()->warn(__VA_ARGS__)
#define SIR_ERROR(...) ::SirMetal::Log::getClientLogger()->error(__VA_ARGS__)
#define SIR_FATAL(...) ::SirMetal::Log::getClientLogger()->fatal(__VA_ARGS__)