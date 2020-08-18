#pragma once

#import <objc/objc.h>
#import "SirMetalLib/graphics/materialManager.h"
#import "SirMetalLib/resources/handle.h"
namespace SirMetal {
class Selection {
 public:
  void initialize(id device);
  void render(id device,
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
