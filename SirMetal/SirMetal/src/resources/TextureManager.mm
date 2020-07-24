
#include "textureManager.h"
#import "handle.h"

namespace SirMetal {

    TextureHandle TextureManager::allocate(id <MTLDevice> device, const AllocTextureRequest &request) {

        auto found = m_nameToHandle.find(request.name);
        if (found != m_nameToHandle.end()) {
            return TextureHandle{found->second};
        }

        MTLTextureDescriptor *textureDescriptor = [[MTLTextureDescriptor alloc] init];
        textureDescriptor.textureType = request.type;
        textureDescriptor.width = request.width;
        textureDescriptor.height = request.height;
        textureDescriptor.sampleCount = request.sampleCount;
        textureDescriptor.pixelFormat = request.format;
        textureDescriptor.usage = request.usage;

        auto tex = [device newTextureWithDescriptor:textureDescriptor];
        TextureHandle handle = getHandle<TextureHandle>(m_textureCounter++);
        m_data[handle.handle] = TextureData{request, tex};
        m_nameToHandle[request.name] = handle.handle;

        return handle;
    }
}
