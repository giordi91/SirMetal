#pragma once
#include <stdint.h>
#include <string>

#include "SirMetal/core/clock.h"
#include "SirMetal/graphics/graphicsDefines.h"

class SDL_Window;
namespace SirMetal {
class Input;
class Window;
class ShaderManager;
class ConstantBufferManager;
class MeshManager;

namespace graphics {
class BufferManager;
class DebugRenderer;
class RenderingContext;
class TextureManager;
} // namespace graphics

struct WindowProps {
  std::string m_title = "Engine Window";
  uint32_t m_width = 1280;
  uint32_t m_height = 720;
  bool m_vsync = false;
  bool m_startFullScreen = false;
};

// this is the high level entry point this is where we start and setup the
// engine
struct EngineConfig {
  // project and IO data config
  std::string m_dataSourcePath;
  WindowProps m_windowConfig;
  // graphics
  uint32_t m_frameBufferingCount;
};

struct Timing {
  GameClock m_clock;
  size_t m_lastFrameTimeNS;
  size_t m_totalNumberOfFrames;
  double m_timeSinceStartInSeconds;
  double m_deltaTimeInSeconds;
  void newFrame();
};

struct EngineContext {
  // Core
  EngineConfig m_config{};
  Window *m_window;
  Timing m_timings;
  uint32_t inFlightFrames = 3;
  // Graphics
  graphics::RenderingContext *m_renderingContext{};
  ShaderManager *m_shaderManager{};
  //TODO convert this to a generic buffer manager
  ConstantBufferManager *m_constantBufferManager{};
  MeshManager *m_meshManager{};
  /*
  graphics::TextureManager *m_textureManager{};
  graphics::DebugRenderer *m_debugRenderer{};
  */
  // Input
  Input *m_inputManager{};
};

EngineConfig loadEngineConfigFile(const std::string& path);
EngineContext *engineStartUp(const EngineConfig &config,SDL_Window* window);
void engineShutdown(EngineContext *context);

} // namespace SirMetal
