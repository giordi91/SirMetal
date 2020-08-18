#pragma once

#import <objc/objc.h>
namespace SirMetal{
class Selection {
 public:
  void initialize();
  void render(id commandBuffer, int screenWidth, int screenHeight);
 private:
  id m_jumpTexture;
  id m_jumpTexture2;
  id m_jumpMaskTexture;
};

}
