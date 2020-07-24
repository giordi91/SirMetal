
#include "textureManager.h"
#import "handle.h"

namespace SirMetal {

    TextureHandle TextureManager::allocate(id <MTLDevice> device, const AllocTextureRequest &request) {

        //auto found = m_nameToHandle.find(request.name);
        //if (found != m_nameToHandle.end()) {
        //    return TextureHandle{found->second};
        //}

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
        //leaving this broken so i don't forget
        change this to check for all depth format
        textureDescriptor.storageMode = request.format == MTLPixelFormatDepth32Float_Stencil8 ? MTLStorageModePrivate: MTLStorageModeManaged;

        auto tex = [device newTextureWithDescriptor:textureDescriptor];
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
}
