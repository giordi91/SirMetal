
#import <cassert>
#import <Metal/Metal.h>
#include "SirMetalLib/core/memory/gpu/GPUMemoryAllocator.h"


namespace SirMetal {

    static const uint32_t bufferAlignmentInBytes = 256;

    inline uint64_t alignUp(uint64_t n, uint32_t alignment) {
        return ((n + alignment - 1) / alignment) * alignment;
    }

    BufferHandle GPUMemoryAllocator::allocate(const uint32_t sizeInBytes,
            const char *name, const BUFFER_FLAGS flags) {
        bool isGpuOnly = (flags & BUFFER_FLAGS_BITS::BUFFER_FLAG_GPU_ONLY) > 0;
        //unsupported flags yet
        assert(!isGpuOnly);

        Buffer bufferData{};
        id <MTLDevice> device = m_device;
        uint32_t allocatedSize = static_cast<uint32_t>(alignUp(sizeof(sizeInBytes), bufferAlignmentInBytes));
        id <MTLBuffer> buffer = [device newBufferWithLength:allocatedSize
                                                    options:MTLResourceOptionCPUCacheModeDefault];
        if(name != nullptr) {
            [buffer setLabel:[NSString stringWithUTF8String:name]];
        }
        bufferData.buffer = buffer;

        bufferData.offset = 0;
        bufferData.range = sizeInBytes;
        bufferData.allocationSize = allocatedSize;

        uint32_t index = versionCounter++;
        m_bufferStorage[index] = bufferData;
        // creating a handle
        return getHandle<BufferHandle>(index);
    }

    void GPUMemoryAllocator::cleanup() {

    }
}
