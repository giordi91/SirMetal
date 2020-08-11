
#include "textureManager.h"
#import "handle.h"
#import "SirMetalLib/core/logging/log.h"

namespace SirMetal {

    bool isDepthFormat(MTLPixelFormat format) {
        return (MTLPixelFormatDepth32Float_Stencil8 == format)
                | (MTLPixelFormatDepth32Float == format)
                | (MTLPixelFormatDepth16Unorm == format)
                | (MTLPixelFormatDepth24Unorm_Stencil8 == format);


    }

    id <MTLTexture> createTextureFromRequest(id <MTLDevice> device, const AllocTextureRequest request) {
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
            //if is a depth format storage mode must be private
            assert(request.storage == MTLStorageModePrivate);
        }
        textureDescriptor.storageMode = request.storage;

        auto tex = [device newTextureWithDescriptor:textureDescriptor];
        return tex;
    }

    TextureHandle TextureManager::allocate(id <MTLDevice> device, const AllocTextureRequest &request) {

        auto found = m_nameToHandle.find(request.name);
        if (found != m_nameToHandle.end()) {
            return TextureHandle{found->second};
        }
        auto tex = createTextureFromRequest(device, request);

        TextureHandle handle = getHandle<TextureHandle>(m_textureCounter++);
        m_data[handle.handle] = TextureData{request, tex};
        m_nameToHandle[request.name] = handle.handle;

        return handle;
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

    bool TextureManager::resizeTexture(id <MTLDevice> device, TextureHandle handle, uint32_t
    newWidth, uint32_t newHeight) {

        //making sure we got a correct handle
        HANDLE_TYPE type = getTypeFromHandle(handle);
        if (type != HANDLE_TYPE::TEXTURE) {
            SIR_CORE_ERROR("[Texture Manager] Provided handle is not a texture handle");
            return false;
        }

        //fetching the corresponding data
        auto found = m_data.find(handle.handle);
        if (found == m_data.end()) {
            SIR_CORE_ERROR("[Texture Manager] Could not find data for requested handle");
            return false;
        }
        TextureData &texData = found->second;
        if ((texData.request.width == newWidth) & (texData.request.height == newHeight)) {
            SIR_CORE_WARN("[Texture Manager] Requested resize of texture with name{} with same size {}x{}", texData.request.name,
                    texData.request.width, texData.request.height);
            return true;
        }
        texData.request.width = newWidth;
        texData.request.height = newHeight;

        //decrease ref to old texture
        texData.texture = nil;

        //alloc new texture
        texData.texture = createTextureFromRequest(device, texData.request);
        if (texData.texture == nil) {
            SIR_CORE_ERROR("[Texture Manager] Could not resize requested texture with name {}",texData.request.name);
            return false;
        }

        return true;

    }
}
