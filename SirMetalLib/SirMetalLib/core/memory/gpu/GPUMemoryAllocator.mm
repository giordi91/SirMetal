
#import <cassert>
#import <Metal/Metal.h>
#import <SirMetalLib/core/logging/log.h>
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
        uint32_t allocatedSize = static_cast<uint32_t>(alignUp(sizeInBytes, bufferAlignmentInBytes));
        id <MTLBuffer> buffer = [device newBufferWithLength:allocatedSize
                                                    options:MTLResourceOptionCPUCacheModeDefault];
        if (name != nullptr) {
            [buffer setLabel:[NSString stringWithUTF8String:name]];
        }
        //populating the bookkeeping data
        bufferData.buffer = buffer;
        bufferData.offset = 0;
        bufferData.range = sizeInBytes;
        bufferData.allocationSize = allocatedSize;
        bufferData.flags = flags;

        uint32_t index = versionCounter++;
        m_bufferStorage[index] = bufferData;
        // creating a handle
        return getHandle<BufferHandle>(index);
    }

    void GPUMemoryAllocator::cleanup() {

    }

    void GPUMemoryAllocator::update(BufferHandle handle, void *data, uint32_t offset, uint32_t size) const {
        assert(getTypeFromHandle(handle) == HANDLE_TYPE::BUFFER);
        uint32_t index = getIndexFromHandle(handle);
        auto found = m_bufferStorage.find(index);
        if(found == m_bufferStorage.end())
        {
            SIR_CORE_ERROR("Tried to update buffer {}, but buffer could not be found",index);
            return;
        }

        const Buffer& bufferData= found->second;
        bool isGPUOnly = (bufferData.flags & BUFFER_FLAG_GPU_ONLY) > 0 ;
        if(isGPUOnly)
        {
            assert(0 && "not supported yet");
        }
        else
        {
            assert(size <= bufferData.allocationSize);
            memcpy((char *) ([bufferData.buffer contents]) + offset, data, size);
        }

    }

    id GPUMemoryAllocator::getBuffer(BufferHandle handle) {
        assert(getTypeFromHandle(handle) == HANDLE_TYPE::BUFFER);
        uint32_t index = getIndexFromHandle(handle);
        auto found = m_bufferStorage.find(index);
        if(found == m_bufferStorage.end())
        {
            SIR_CORE_ERROR("Tried to get metal buffer from handle {}, but buffer could not be found",index);
            return nil;
        }
        return m_bufferStorage[index].buffer;
    }
}
