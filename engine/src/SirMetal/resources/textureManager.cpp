
#include "SirMetal/resources/textureManager.h"
#include "SirMetal/resources/handle.h"
#include <SirMetal/resources/textures/gltfTexture.h>

namespace SirMetal {

bool isDepthFormat(MTLPixelFormat format) {
  return (MTLPixelFormatDepth32Float_Stencil8 == format) |
         (MTLPixelFormatDepth32Float == format) |
         (MTLPixelFormatDepth16Unorm == format) |
         (MTLPixelFormatDepth24Unorm_Stencil8 == format);
}


id<MTLTexture> createTextureFromRequest(id<MTLDevice> device,
                                        const AllocTextureRequest &request) {
  if (request.sampleCount != 1) {
    assert(request.mipLevel == 1 && "if we use MSAA we can't have mip maps");
  }

  MTLTextureDescriptor *textureDescriptor = [[MTLTextureDescriptor alloc] init];
  textureDescriptor.textureType = request.type;
  textureDescriptor.width = request.width;
  textureDescriptor.height = request.height;
  textureDescriptor.sampleCount = request.sampleCount;
  textureDescriptor.pixelFormat = request.format;
  textureDescriptor.mipmapLevelCount = request.mipLevel;
  textureDescriptor.usage = request.usage;
  if (isDepthFormat(request.format)) {
    // if is a depth format storage mode must be private
    assert(request.storage == MTLStorageModePrivate);
  }
  textureDescriptor.storageMode = request.storage;

  auto tex = [device newTextureWithDescriptor:textureDescriptor];
  return tex;
}

TextureHandle TextureManager::allocate(id<MTLDevice> device,
                                       const AllocTextureRequest &request) {

  auto found = m_nameToHandle.find(request.name);
  if (found != m_nameToHandle.end()) {
    return TextureHandle{found->second};
  }
  auto tex = createTextureFromRequest(device, request);

  auto handle = getHandle<TextureHandle>(m_textureCounter++);
  m_data[handle.handle] = TextureData{request, tex};
  m_nameToHandle[request.name] = handle.handle;

  return handle;
}

TextureHandle TextureManager::loadFromMemory(id<MTLDevice> device, id<MTLCommandQueue> queue, void *data, LOAD_TEXTURE_TYPE type, bool isGamma) {
  switch (type) {
    case LOAD_TEXTURE_TYPE::INVALID: {
      assert(0 && "unsupported texture");
      break;
    }
    case LOAD_TEXTURE_TYPE::GLTF_TEXTURE: {
      TextureLoadResult outData;
      loadGltfTexture(outData, data, isGamma);
      return createTextureFromTextureLoadResult(device, queue, outData);
    }
  }
  return {};
}

id TextureManager::getNativeFromHandle(TextureHandle handle) {
  HANDLE_TYPE type = getTypeFromHandle(handle);
  assert(type == HANDLE_TYPE::TEXTURE);
  auto found = m_data.find(handle.handle);
  if (found != m_data.end()) {
    return found->second.texture;
  }
  assert(0 && "requested invalid texture");
  return nil;
}

bool TextureManager::resizeTexture(id<MTLDevice> device, TextureHandle handle,
                                   uint32_t newWidth, uint32_t newHeight) {

  // making sure we got a correct handle
  HANDLE_TYPE type = getTypeFromHandle(handle);
  if (type != HANDLE_TYPE::TEXTURE) {
    printf("[ERROR][Texture Manager] Provided handle is not a texture handle");
    return false;
  }

  // fetching the corresponding data
  auto found = m_data.find(handle.handle);
  if (found == m_data.end()) {
    printf("[ERROR][Texture Manager] Could not find data for requested handle");
    return false;
  }
  TextureData &texData = found->second;
  if ((texData.request.width == newWidth) &
      (texData.request.height == newHeight)) {
    printf("[WARN][Texture Manager] Requested resize of texture with name%s "
           "with same size %ix%i",
           texData.request.name.c_str(), texData.request.width,
           texData.request.height);
    return true;
  }
  texData.request.width = newWidth;
  texData.request.height = newHeight;

  // decrease ref to old texture
  texData.texture = nil;

  // alloc new texture
  texData.texture = createTextureFromRequest(device, texData.request);
  if (texData.texture == nil) {
    printf("[ERROR][Texture Manager] Could not resize requested texture with "
           "name %s",
           texData.request.name.c_str());
    return false;
  }

  return true;
}


TextureHandle TextureManager::createTextureFromTextureLoadResult(
        id<MTLDevice> device, id<MTLCommandQueue> queue, const TextureLoadResult &result) {

  MTLTextureDescriptor *textureDescriptor = [[MTLTextureDescriptor alloc] init];
  textureDescriptor.textureType = result.isCube ? MTLTextureType::MTLTextureTypeCube : MTLTextureType::MTLTextureType2D;
  textureDescriptor.width = result.width;
  textureDescriptor.height = result.height;
  textureDescriptor.sampleCount = 1;//no msaa texture from file
  textureDescriptor.pixelFormat = resultToMetalPixelFormat(result.format);
  textureDescriptor.mipmapLevelCount = result.mipLevel;
  textureDescriptor.usage = MTLTextureUsageShaderRead;
  textureDescriptor.storageMode = MTLStorageModeManaged;//for now only supporting private mode


  id<MTLTexture> texStaging = [device
          newTextureWithDescriptor:textureDescriptor];

  [texStaging replaceRegion:MTLRegionMake2D(0, 0, result.width, result.height)
                mipmapLevel:0
                  withBytes:result.data.data()
                bytesPerRow:sizeof(uint32_t) * result.width];

  textureDescriptor.storageMode = MTLStorageModePrivate;
  textureDescriptor.mipmapLevelCount = result.mipLevel;
  int mipCount = std::floor(std::log2(std::max(result.width, result.height))) + 1;
  textureDescriptor.mipmapLevelCount = mipCount;

  id<MTLTexture> tex = [device
          newTextureWithDescriptor:textureDescriptor];

  id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
  id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
  [commandEncoder copyFromTexture:texStaging toTexture:tex];
  [commandEncoder generateMipmapsForTexture:tex];
  [commandEncoder endEncoding];
  [commandBuffer commit];


  TextureData data{};
  data.request.width = result.width;
  data.request.height = result.height;
  data.request.sampleCount = textureDescriptor.sampleCount;
  data.request.type = textureDescriptor.textureType;
  data.request.format = textureDescriptor.pixelFormat;
  data.request.usage = textureDescriptor.usage;
  data.request.storage = textureDescriptor.storageMode;
  data.request.mipLevel = textureDescriptor.mipmapLevelCount;
  data.request.name = result.name;
  data.texture = tex;

  auto handle = getHandle<TextureHandle>(m_textureCounter++);
  m_data[handle.handle] = data;
  m_nameToHandle[data.request.name] = handle.handle;

  return handle;
}
MTLPixelFormat TextureManager::resultToMetalPixelFormat(LOAD_TEXTURE_PIXEL_FORMAT format) {
  switch (format) {
    case LOAD_TEXTURE_PIXEL_FORMAT::INVALID: {
      assert(0 && "passed invalid pixel format");
      return MTLPixelFormat::MTLPixelFormatInvalid;
    }
    case LOAD_TEXTURE_PIXEL_FORMAT::RGBA32_UNORM: {
      return MTLPixelFormatRGBA8Unorm;
    }
    case LOAD_TEXTURE_PIXEL_FORMAT::RGBA32_UNORM_S: {
      return MTLPixelFormatRGBA8Unorm_sRGB;
    }
  }
}
TextureHandle TextureManager::generateSolidColorTexture(id<MTLDevice> device, id<MTLCommandQueue> queue, int w, int h, uint32_t color, const std::string &name) {


  // Create a render target which the shading kernel can write to
  MTLTextureDescriptor *textureDescriptor =
          [[MTLTextureDescriptor alloc] init];

  textureDescriptor.textureType = MTLTextureType2D;
  textureDescriptor.width = w;
  textureDescriptor.height = h;

  // Stored in private memory because it will only be read and written from the
  // GPU
  textureDescriptor.storageMode = MTLStorageModeManaged;

  textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
  textureDescriptor.usage = MTLTextureUsageShaderRead;

  id<MTLTexture> texStaging = [device
          newTextureWithDescriptor:textureDescriptor];

  auto *values =
          static_cast<uint32_t *>(malloc(sizeof(uint32_t) * w * h));

  for (NSUInteger i = 0; i < w * h; i++)
    values[i] = color;

  [texStaging replaceRegion:MTLRegionMake2D(0, 0, w, h)
                mipmapLevel:0
                  withBytes:values
                bytesPerRow:sizeof(uint32_t) * w];
  free(values);

  textureDescriptor.storageMode = MTLStorageModePrivate;
  int mipCount = static_cast<int>(std::floor(std::log2(std::max(w, h) + 1)));
  mipCount = std::max(mipCount,1);
  textureDescriptor.mipmapLevelCount = mipCount;

  id<MTLTexture> tex = [device
          newTextureWithDescriptor:textureDescriptor];


  id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
  id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
  [commandEncoder copyFromTexture:texStaging toTexture:tex];
  if(mipCount > 1){
    [commandEncoder generateMipmapsForTexture:tex];
  }
  [commandEncoder endEncoding];
  [commandBuffer commit];


  TextureData data{};
  data.request.width = w;
  data.request.height = h;
  data.request.sampleCount = 1;
  data.request.type = textureDescriptor.textureType;
  data.request.format = textureDescriptor.pixelFormat;
  data.request.usage = textureDescriptor.usage;
  data.request.storage = textureDescriptor.storageMode;
  data.request.mipLevel = textureDescriptor.mipmapLevelCount;
  data.request.name = name;
  data.texture = tex;

  auto handle = getHandle<TextureHandle>(m_textureCounter++);
  m_data[handle.handle] = data;
  m_nameToHandle[data.request.name] = handle.handle;
  return handle;
}
void TextureManager::initialize(id<MTLDevice> device, id<MTLCommandQueue> queue) {
  m_whiteTexture = generateSolidColorTexture(device, queue, 2, 2, 0xFFFFFFFF, "white");
  m_blackTexture = generateSolidColorTexture(device, queue, 2, 2, 0, "black");
}
}// namespace SirMetal
