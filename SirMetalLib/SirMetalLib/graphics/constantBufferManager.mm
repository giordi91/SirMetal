#import <SirMetalLib/engineContext.h>
#include "SirMetalLib/graphics/constantBufferManager.h"


namespace SirMetal {

    static const uint32_t bufferAlignment = 256;

    uint32_t toMultipleOfAlignment(uint32_t size) {
        return ((size + bufferAlignment - 1) / bufferAlignment) * bufferAlignment;
    }

    ConstantBufferHandle ConstantBufferManager::allocate(uint32_t size, CONSTANT_BUFFER_FLAGS flags) {
        bool isBuffered = (flags & CONSTANT_BUFFER_FLAG_BUFFERED) > 0;
        auto multiple = toMultipleOfAlignment(size);
        auto buffSize = multiple * CONTEXT->inFlightFrames;
        uint32_t requestedSize = isBuffered ? buffSize : size;
        uint32_t allocIndex = findAllocator(requestedSize);
        PoolTracker &allocator = m_bufferPools[allocIndex];
        BufferRangeHandle rangeHandle = allocator.linearManager->allocate(requestedSize, bufferAlignment);
        BufferRange range = allocator.linearManager->getBufferRange(rangeHandle);

        uint32_t index = static_cast<uint32_t>(m_constBuffers.size());
        m_constBuffers.emplace_back(ConstantBufferData{range, allocIndex, size, flags});
        return getHandle<ConstantBufferHandle>(index);
    }

    void ConstantBufferManager::update(ConstantBufferHandle handle, void *data) {
        assert(getTypeFromHandle(handle) == HANDLE_TYPE::CONSTANT_BUFFER);
        int dataIndex = getIndexFromHandle(handle);
        ConstantBufferData &constantData = m_constBuffers[dataIndex];
        bool isBuffered = (constantData.flags & CONSTANT_BUFFER_FLAG_BUFFERED) > 0;
        uint32_t currentFrame = CONTEXT->frame % CONTEXT->inFlightFrames;
        uint32_t perFrameOffset = toMultipleOfAlignment(constantData.userRequestedSize) * currentFrame;
        uint32_t finalOffset = static_cast<uint32_t>(constantData.range.m_offset + perFrameOffset * isBuffered);

        auto bufferHandle = m_bufferPools[constantData.allocIndex].bufferHandle;
        m_allocator.update(bufferHandle, data, finalOffset, constantData.userRequestedSize);

    }

    void ConstantBufferManager::initialize(id device, uint32_t poolSize) {
        m_allocator.initialize(device);
        m_poolSize = poolSize;
        allocateBufferPool();
    }

    void ConstantBufferManager::allocateBufferPool() {
        uint32_t index = static_cast<uint32_t>(m_bufferPools.size());
        char printBuffer[64];
        sprintf(printBuffer, "constantBufferPool%d", index);
        BufferHandle buffer = m_allocator.allocate(m_poolSize, printBuffer,
                BUFFER_FLAG_NONE);
        LinearBufferManager *tracker = new LinearBufferManager(m_poolSize);
        m_bufferPools.emplace_back(PoolTracker{buffer, tracker});
    }

    uint32_t ConstantBufferManager::findAllocator(uint32_t size) {
        size_t count = m_bufferPools.size();
        for (size_t i = 0; i < count; ++i) {
            if (m_bufferPools[i].linearManager->canAllocate(size)) {
                return static_cast<uint32_t>(i);
            }
        }
        //if we are here it means we need a new buffer
        allocateBufferPool();
        return static_cast<uint32_t>(m_bufferPools.size() - 1);
    }

    BindInfo ConstantBufferManager::getBindInfo(const ConstantBufferHandle handle) {

        assert(getTypeFromHandle(handle) == HANDLE_TYPE::CONSTANT_BUFFER);
        int dataIndex = getIndexFromHandle(handle);
        ConstantBufferData &constantData = m_constBuffers[dataIndex];
        bool isBuffered = (constantData.flags & CONSTANT_BUFFER_FLAG_BUFFERED) > 0;
        auto bufferHandle = m_bufferPools[constantData.allocIndex].bufferHandle;

        uint32_t currentFrame = CONTEXT->frame % CONTEXT->inFlightFrames;
        uint32_t perFrameOffset = toMultipleOfAlignment(constantData.userRequestedSize) * currentFrame;
        uint32_t finalOffset = static_cast<uint32_t>(constantData.range.m_offset + perFrameOffset * isBuffered);

        return {m_allocator.getBuffer(bufferHandle),
                static_cast<uint32_t>(finalOffset),
                static_cast<uint32_t>(constantData.range.m_size)};
    }
}