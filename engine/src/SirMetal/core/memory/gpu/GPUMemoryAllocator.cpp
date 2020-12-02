
#include "SirMetal/core/memory/gpu/GPUMemoryAllocator.h"
#import <Metal/Metal.h>
#import <cassert>

namespace SirMetal {

static const uint32_t bufferAlignmentInBytes = 256;

inline uint64_t alignUp(uint64_t n, uint32_t alignment) {
  return ((n + alignment - 1) / alignment) * alignment;
}

BufferHandle GPUMemoryAllocator::allocate(const uint32_t sizeInBytes,
                                          const char *name,
                                          const BUFFER_FLAGS flags,
                                          void *data) {
  bool isGpuOnly = (flags & BUFFER_FLAGS_BITS::BUFFER_FLAG_GPU_ONLY) > 0;
  // unsupported flags yet
  // assert(!isGpuOnly);

  Buffer bufferData{};
  id<MTLDevice> device = m_device;
  uint32_t allocatedSize =
      static_cast<uint32_t>(alignUp(sizeInBytes, bufferAlignmentInBytes));

  id<MTLBuffer> buffer;
  // let us take care of the main buffer
  if (isGpuOnly) {
    buffer = [device newBufferWithLength:allocatedSize
                                 options:MTLResourceStorageModePrivate];
  } else {
    // if not gpu only, it will be cpu visible.
    // if data is provided it means we can create the cpu buffer with
    // some initial data, otherwise we create an empty one
    if (data == nullptr) {
      buffer =
          [device newBufferWithLength:allocatedSize
                              options:MTLResourceOptionCPUCacheModeDefault];
    } else {
      buffer = [device newBufferWithBytes:data
                                   length:allocatedSize
                                  options:MTLResourceOptionCPUCacheModeDefault];
    }
  }
  if (name != nullptr) {
    [buffer setLabel:[NSString stringWithUTF8String:name]];
  }

  // the only use case that has not been handled is the case where we have a gpu
  // only buffer but we also want some data in it, like it can be a mesh buffer
  // if that is the case we are going to create a temporary buffer, cpu visible
  // and dispatch a gpu to gpu copy
  if (isGpuOnly & (data != nullptr)) {
    // we need a temporary buffer
    id<MTLBuffer> sharedBuffer =
        [device newBufferWithBytes:data
                            length:sizeInBytes
                           options:MTLResourceOptionCPUCacheModeDefault];

    // Create a command buffer for GPU work.
    id<MTLCommandQueue> queue = m_queue;
    id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];

    // Encode a blit pass to copy data from the source buffer to the private
    // buffer.
    id<MTLBlitCommandEncoder> blitCommandEncoder =
        [commandBuffer blitCommandEncoder];
    [blitCommandEncoder copyFromBuffer:sharedBuffer
                          sourceOffset:0
                              toBuffer:buffer
                     destinationOffset:0
                                  size:sizeInBytes];
    // TODO submitting a single command buffer for one command is not great
    // in case, we might have to improve this solution, use some batching of
    // sort
    [blitCommandEncoder endEncoding];
    [commandBuffer commit];
  }

  // populating the bookkeeping data
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

void GPUMemoryAllocator::cleanup() {}

void GPUMemoryAllocator::update(BufferHandle handle, void *data,
                                uint32_t offset, uint32_t size) const {
  assert(getTypeFromHandle(handle) == HANDLE_TYPE::BUFFER);
  uint32_t index = getIndexFromHandle(handle);
  auto found = m_bufferStorage.find(index);
  if (found == m_bufferStorage.end()) {
    printf("[ERROR] Tried to update buffer %i, but buffer could not be found",
           index);
    return;
  }

  const Buffer &bufferData = found->second;
  bool isGPUOnly = (bufferData.flags & BUFFER_FLAG_GPU_ONLY) > 0;
  if (isGPUOnly) {
    assert(0 && "not supported yet");
  } else {
    assert(size <= bufferData.allocationSize);
    memcpy((char *)([bufferData.buffer contents]) + offset, data, size);
  }
}

id GPUMemoryAllocator::getBuffer(BufferHandle handle) {
  assert(getTypeFromHandle(handle) == HANDLE_TYPE::BUFFER);
  uint32_t index = getIndexFromHandle(handle);
  auto found = m_bufferStorage.find(index);
  if (found == m_bufferStorage.end()) {
    printf("[ERROR] Tried to get metal buffer from handle %i, but buffer could "
           "not be found",
           index);
    return nil;
  }
  return m_bufferStorage[index].buffer;
}
} // namespace SirMetal
