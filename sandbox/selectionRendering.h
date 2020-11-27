#pragma once

#import <objc/objc.h>
#import "SirMetal/graphics/materialManager.h"
#import "SirMetal/resources/handle.h"
namespace SirMetal {
struct EngineContext;
class Selection {
 public:
  void initialize(EngineContext* context );
  void render(EngineContext* context ,
              id commandBuffer,
              int screenWidth,
              int screenHeight,
              ConstantBufferHandle uniformHandle,
              id offscreenTexture);
 private:
  id m_jumpTexture;
  id m_jumpTexture2;
  id m_jumpMaskTexture;
  ConstantBufferHandle m_floodCB;
  Material m_jumpFloodMaterial;
  Material m_jumpFloodMaskMaterial;
  Material m_jumpFloodInitMaterial;
  Material m_jumpOutlineMaterial;

  TextureHandle m_jumpHandle;
  TextureHandle m_jumpHandle2;
  TextureHandle m_jumpMaskHandle;

  uint32_t m_screenWidth = 256;
  uint32_t m_screenHeight = 256;
};

}
