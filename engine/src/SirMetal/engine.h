#pragma once
#include <stdint.h>
#include <string>

#include "SirMetal/core/clock.h"
#include "SirMetal/graphics/graphicsDefines.h"

namespace SirMetal {
class Input;
class SDLWindow;

namespace graphics {
class BufferManager;
class DebugRenderer;
class ShaderManager;
class RenderingContext;
class MeshManager;
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
  SDLWindow *m_window;
  Timing m_timings;
  /*
  // Graphics
  graphics::RenderingContext *m_renderingContext{};
  graphics::ShaderManager *m_shaderManager{};
  graphics::MeshManager *m_meshManager{};
  graphics::TextureManager *m_textureManager{};
  graphics::BufferManager *m_bufferManager{};
  graphics::DebugRenderer *m_debugRenderer{};
  */
  // Input
  Input *m_inputManager{};
};

EngineConfig loadEngineConfigFile(const std::string& path);
EngineContext *engineStartUp(const EngineConfig &config);
void engineShutdown(EngineContext *context);

} // namespace SirMetal
