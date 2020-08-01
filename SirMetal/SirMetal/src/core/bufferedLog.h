#pragma once

#include <spdlog/sinks/base_sink.h>
#include <string>
#include <iostream>

namespace SirMetal {
    template<typename Mutex>
    class BufferedSink final : public spdlog::sinks::base_sink<Mutex> {

    protected:
        void sink_it_(const spdlog::details::log_msg &msg) override {
            spdlog::memory_buf_t formatted;
            spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
            buffer += fmt::to_string(formatted) ;
            buffer += "\n";
        }

        void flush_() override {
            //buffer.clear();
        }

    public:
        const std::string* getBuffer() const {
            return &buffer;
        }

    private:
        std::string buffer;
    };

    using BufferedSink_mt = BufferedSink<std::mutex>;
    using BufferedSink_st = BufferedSink<spdlog::details::null_mutex>;
} // namespace sinks
