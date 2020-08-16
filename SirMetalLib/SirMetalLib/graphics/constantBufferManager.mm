#include "SirMetalLib/graphics/constantBufferManager.h"


namespace SirMetal
{


    ConstantBufferHandle ConstantBufferManager::allocate(uint32_t size, CONSTANT_BUFFER_FLAGS flags) {
        return ConstantBufferHandle();
    }

    void ConstantBufferManager::update(ConstantBufferHandle handle, void *data) {

    }
}