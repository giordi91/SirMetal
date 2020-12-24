#pragma once

#include <assert.h>
#include <stdint.h>
#include <string>
#include <unordered_map>

#import <Metal/Metal.h>

#import "../../../../../../../../../Library/Developer/CommandLineTools/SDKs/MacOSX11.1.sdk/System/Library/Frameworks/Metal.framework/Headers/MTLPixelFormat.h"
#include "SirMetal/resources/handle.h"
#include "SirMetal/resources/resourceTypes.h"
#import "gltfLoader.h"
#import "handle.h"
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
  void initialize(id<MTLDevice> device);
  void cleanup(){};

  TextureHandle allocate(id<MTLDevice> device,
                         const AllocTextureRequest &request);
  TextureHandle loadFromMemory(id<MTLDevice> device, id<MTLCommandQueue> queue, void *data, LOAD_TEXTURE_TYPE type, bool isGamma);

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

  TextureHandle getWhiteTexture() const { return m_whiteTexture; }
  TextureHandle getBlackTexture() const { return m_blackTexture; }

  private:
  TextureHandle createTextureFromTextureLoadResult(id<MTLDevice> device, id<MTLCommandQueue> queue, const TextureLoadResult &result);
  MTLPixelFormat resultToMetalPixelFormat(LOAD_TEXTURE_PIXEL_FORMAT format);
  TextureHandle generateSolidColorTexture(id<MTLDevice> device,  int w, int h, uint32_t color, const std::string &name);

  private:
  struct TextureData {
    AllocTextureRequest request;
    id<MTLTexture> texture;
  };

  private:
  std::unordered_map<uint32_t, TextureData> m_data;
  std::unordered_map<std::string, uint32_t> m_nameToHandle;
  int m_textureCounter = 1;
  TextureHandle m_whiteTexture{};
  TextureHandle m_blackTexture{};
};
}// namespace SirMetal
