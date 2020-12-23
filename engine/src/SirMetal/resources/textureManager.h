#pragma once

#include <assert.h>
#include <stdint.h>
#include <string>
#include <unordered_map>

#import <Metal/Metal.h>

#include "SirMetal/resources/handle.h"
#include "SirMetal/resources/resourceTypes.h"
#import "gltfLoader.h"
#import "resourceTypes.h"

namespace SirMetal {
struct AllocTextureRequest {
  uint32_t width;
  uint32_t height;
  uint32_t sampleCount;
  MTLTextureType type;
  MTLPixelFormat format;
  MTLTextureUsage usage;
  MTLStorageMode storage;
  uint32_t mipLevel = 1;
  std::string name;
};

class TextureManager {
public:
  TextureManager() = default;
  void initialize(){};
  void cleanup(){};

  TextureHandle allocate(id<MTLDevice> device,
                         const AllocTextureRequest &request);
  TextureHandle loadFromMemory(void *data, LOAD_TEXTURE_TYPE type,
                               bool isGamma);

  bool resizeTexture(id<MTLDevice> device, TextureHandle handle,
                     uint32_t newWidth, uint32_t newHeight);

  MTLPixelFormat getFormat(const TextureHandle handle) const {
    HANDLE_TYPE type = getTypeFromHandle(handle);
    assert(type == HANDLE_TYPE::TEXTURE);
    auto found = m_data.find(handle.handle);
    if (found != m_data.end()) {
      return found->second.request.format;
    }
    assert(0 && "requested invalid texture");
    return MTLPixelFormatInvalid;
  }

  id getNativeFromHandle(TextureHandle handle);

private:
  TextureHandle createTextureFromTextureLoadResult(const TextureLoadResult &result);

private:
  struct TextureData {
    AllocTextureRequest request;
    id<MTLTexture> texture;
  };

private:
  std::unordered_map<uint32_t, TextureData> m_data;
  std::unordered_map<std::string, uint32_t> m_nameToHandle;
  int m_textureCounter = 1;
};
} // namespace SirMetal
