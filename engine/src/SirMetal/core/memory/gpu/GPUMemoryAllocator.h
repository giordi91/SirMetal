#pragma once

#import <objc/objc.h>
#import <stdint.h>
#import <unordered_map>

#import "SirMetal/resources/handle.h"

namespace SirMetal {
enum BUFFER_FLAGS_BITS {
  BUFFER_FLAG_NONE = 0,
  BUFFER_FLAG_GPU_ONLY = 1,
};

typedef uint32_t BUFFER_FLAGS;

class GPUMemoryAllocator {

public:
  GPUMemoryAllocator() = default;
  ~GPUMemoryAllocator() = default;

  // deleted method to avoid copy, you can still move it though
  GPUMemoryAllocator(const GPUMemoryAllocator &) = delete;

  GPUMemoryAllocator &operator=(const GPUMemoryAllocator &) = delete;

  void initialize(id device, id queue) { m_device = device; m_queue = queue;};

  void cleanup();

  BufferHandle allocate(const uint32_t sizeInBytes, const char *name,
                        const BUFFER_FLAGS flags, void *data = nullptr);

  void update(BufferHandle handle, void *data, uint32_t offset,
              uint32_t size) const;

  /*
  void free(BufferHandle handle) override;

  void *getMappedData(const BufferHandle handle) const override;
   */

  /*
  const vk::Buffer &getBufferData(const BufferHandle handle) const {
      assertMagicNumber(handle);
      uint32_t idx = getIndexFromHandle(handle);
      return m_bufferStorage.getConstRef(idx);
  }
   */
  id getBuffer(BufferHandle handle) const;

private:
  struct Buffer {
    id buffer;
    void *data;
    uint32_t size;
    uint32_t offset;
    uint32_t range;
    uint32_t allocationSize;
    BUFFER_FLAGS flags;
  };

private:
  std::unordered_map<uint32_t, Buffer> m_bufferStorage;
  uint32_t versionCounter = 1;
  id m_device;
  id m_queue;
};
} // namespace SirMetal
