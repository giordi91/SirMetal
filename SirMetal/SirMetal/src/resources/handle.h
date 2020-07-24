
#pragma once

#include <stdint.h>

namespace SirMetal {

    enum class HANDLE_TYPE {
        NONE = 0,
        TEXTURE = 1,
        SHADER_LIBRARY = 2,
        MESH = 3,
    };

    template<typename T>
    inline T getHandle(const uint32_t index) {
        return {(static_cast<uint32_t>(T::type) << 24)| index};
    }

    template<typename T>
    inline uint32_t getIndexFromHandle(const T h) {
        constexpr uint32_t standardIndexMask = (1 << 24) - 1;
        return h.handle & standardIndexMask;
    }

    template<typename T>
    inline uint32_t getTypeFromHandle(const T h) {
        constexpr uint32_t standardIndexMask = (1 << 8) - 1;
        const uint32_t standardMagicNumberMask = ~standardIndexMask;
        return (h.handle & standardMagicNumberMask) >> 24;
    }

    struct TextureHandle final {
        uint32_t handle;

        [[nodiscard]] bool isHandleValid() const {
            return handle != 0;
        }
        static const HANDLE_TYPE type = HANDLE_TYPE::TEXTURE;
    };

    struct LibraryHandle final {
        uint32_t handle;

        [[nodiscard]] bool isHandleValid() const {
            return handle != 0;
        }
        static const HANDLE_TYPE type = HANDLE_TYPE::SHADER_LIBRARY;
    };
}