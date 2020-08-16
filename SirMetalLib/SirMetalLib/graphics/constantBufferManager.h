#pragma once

#import <cstdint>
#import <SirMetalLib/core/memory/gpu/GPUMemoryAllocator.h>

namespace SirMetal {
    enum CONSTANT_BUFFER_FLAGS_BITS {
        CONSTANT_BUFFER_FLAG_RANDOM_NONE = 0,
        CONSTANT_BUFFER_FLAG_BUFFERED = 1,
    };
    typedef uint32_t CONSTANT_BUFFER_FLAGS;

    class ConstantBufferManager {
    public:
        void initialize(id device) {
            m_allocator.initialize(device);
        }
        ConstantBufferHandle allocate(uint32_t size, CONSTANT_BUFFER_FLAGS flags);
        void update(ConstantBufferHandle handle, void* data);
        void cleanup(){};
        // deleted method to avoid copy, you can still move it though
        ConstantBufferManager(const ConstantBufferManager &) = delete;
        ConstantBufferManager &operator=(const ConstantBufferManager &) = delete;
    private:
        GPUMemoryAllocator m_allocator;

    };


}