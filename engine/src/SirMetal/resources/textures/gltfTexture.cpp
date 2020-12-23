#include "SirMetal/resources/textures/gltfTexture.h"

#include <cgltf/cgltf.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace SirMetal {

LOAD_TEXTURE_PIXEL_FORMAT getGltfTextureFormat(const cgltf_texture *pTexture,
                                               bool isGamma) {
  if (strcmp(pTexture->image->mime_type, "image/png") == 0) {
    return isGamma ? LOAD_TEXTURE_PIXEL_FORMAT::RGBA32_UNORM_S
                   : LOAD_TEXTURE_PIXEL_FORMAT::RGBA32_UNORM;
  }
  assert(0 && "cannot deduce pixel format from gltf texture");
  return LOAD_TEXTURE_PIXEL_FORMAT::INVALID;
}


void loadGltfTexture(TextureLoadResult &outData, void *data, bool isGamma) {
  const auto *texture = reinterpret_cast<const cgltf_texture *>(data);
  outData.name = texture->image->name;

  const auto *buffer = texture->image->buffer_view->buffer;
  assert(texture->image->uri == nullptr);
  assert(buffer != nullptr);

  //for now we expect the texture to be baked in the file
  int x,y,channels;
  int requestedChannels =4;
  auto* ptr = stbi_load_from_memory((stbi_uc*)buffer->data + texture->image->buffer_view->offset,buffer->size,&x,&y,&channels,requestedChannels);
  assert(ptr != nullptr && "failed to load png or jpg file");


  size_t bufferSize = sizeof(char)*requestedChannels*x*y;
  outData.data.resize(bufferSize);
  memcpy(outData.data.data(), ptr, bufferSize);

  outData.format = getGltfTextureFormat(texture, isGamma);
  outData.hasMips = false;
  outData.isCube = false;
}

} // namespace SirMetal
