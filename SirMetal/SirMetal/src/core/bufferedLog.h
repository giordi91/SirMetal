#pragma once

#include <spdlog/sinks/base_sink.h>
#include <string>
#include <iostream>

namespace SirMetal {
    template<typename Mutex>
    class BufferedSink final : public spdlog::sinks::base_sink<Mutex> {

    protected:
        void sink_it_(const spdlog::details::log_msg &msg) override {
            buffer += fmt::to_string(msg.payload);
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
