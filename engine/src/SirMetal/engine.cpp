#include "SirMetal/engine.h"

#include <unordered_map>
#include "nlohmann/json.hpp"

#include "SirMetal/io/fileUtils.h"
#include "SirMetal/io/json.h"
#include "SirMetal/core/input.h"
#include "SirMetal/graphics/renderingContext.h"
#include "SirMetal/resources/shaderManager.h"
/*
#include "blackHole/graphics/meshManager.h"
#include "blackHole/graphics/textureManager.h"
#include "blackHole/io/fileUtils.h"
#include "graphics/buffermanager.h"
#include "graphics/debugRenderer.h"
 */

namespace SirMetal{

static const std::string CONFIG_DATA_SOURCE_KEY = "dataSource";
static const std::string CONFIG_START_FULL_SCREEN = "startFullScreen";
static const std::string CONFIG_WINDOW_TITLE = "windowTitle";
static const std::string CONFIG_WINDOW_WIDTH = "windowWidth";
static const std::string CONFIG_WINDOW_HEIGHT = "windowHeight";
static const std::string CONFIG_FRAME_BUFFERING_COUNT = "frameBufferingCount";

static const std::string DEFAULT_STRING = "";

EngineConfig parseConfigFile(const std::string &path) {
  nlohmann::json jobj;
  getJsonObj(path, jobj);

  // start to process the config file
  EngineConfig config{};

  config.m_dataSourcePath =
      getValueIfInJson(jobj, CONFIG_DATA_SOURCE_KEY, DEFAULT_STRING);
  assert(!config.m_dataSourcePath.empty());

  config.m_windowConfig.m_startFullScreen =
      getValueIfInJson(jobj, CONFIG_START_FULL_SCREEN, false);

  // window stuff
  config.m_windowConfig.m_title =
      getValueIfInJson(jobj, CONFIG_WINDOW_TITLE, DEFAULT_STRING);
  config.m_windowConfig.m_width =
      getValueIfInJson(jobj, CONFIG_WINDOW_WIDTH, 0);
  config.m_windowConfig.m_height =
      getValueIfInJson(jobj, CONFIG_WINDOW_HEIGHT, 0);

  config.m_frameBufferingCount =
      (getValueIfInJson(jobj, CONFIG_FRAME_BUFFERING_COUNT, 2u));

  assert(config.m_windowConfig.m_width != 0);
  assert(config.m_windowConfig.m_height != 0);
  return config;
}

EngineConfig initializeConfigDefault() {
  // initialize basic engine allocators
  // start to process the config file
  EngineConfig config;

  config.m_dataSourcePath = "../data/";
  config.m_windowConfig.m_title = "SirMetal";
  config.m_windowConfig.m_width = 1280;
  config.m_windowConfig.m_height = 720;
  config.m_frameBufferingCount = 2;
  return config;
}
EngineConfig loadEngineConfigFile(const std::string &path) {
  const bool exists = fileExists(path);
  if (exists) {
    return parseConfigFile(path);
  }
  printf("[WARN] Engine config not found, initializing default...");
  return initializeConfigDefault();
}

EngineContext *engineStartUp(const EngineConfig &config,SDL_Window *window) {

  auto *context = new EngineContext{};
  context->m_config = config;
  context->m_inputManager = new Input();
  context->m_inputManager->initialize();
  context->m_renderingContext = new graphics::RenderingContext();
  context->m_renderingContext->initialize(config,window);
  id<MTLDevice> device = context->m_renderingContext->getDevice();
  context->m_shaderManager = new ShaderManager();
  context->m_shaderManager->initialize(device);
  /*
  context->m_meshManager = new graphics::MeshManager();
  context->m_meshManager->initialize();
  context->m_bufferManager = new graphics::BufferManager();
  context->m_bufferManager->initialize();
  context->m_textureManager = new graphics::TextureManager();
  context->m_textureManager->initialize(context->m_renderingContext);
  context->m_debugRenderer = new graphics::DebugRenderer();
  context->m_debugRenderer->initialize(context);
   */
  return context;
}

void Timing::newFrame() {
  m_lastFrameTimeNS = m_clock.getDelta();
  ++m_totalNumberOfFrames;
  m_deltaTimeInSeconds = m_lastFrameTimeNS * NS_TO_SECONDS;
  m_timeSinceStartInSeconds = m_clock.getDeltaFromOrigin() * NS_TO_SECONDS;
}
void engineShutdown(EngineContext *context) {
  context->m_inputManager->cleanup();
  delete context->m_inputManager;
  context->m_shaderManager->cleanup();
  delete context->m_shaderManager;
  /*
  context->m_debugRenderer->cleanup(context);
  delete context->m_debugRenderer;
  context->m_textureManager->cleanup();
  delete context->m_textureManager;
  context->m_bufferManager->cleanup();
  delete context->m_bufferManager;
  context->m_meshManager->cleanup();
  delete context->m_meshManager;
  context->m_renderingContext->cleanup();
  delete context->m_renderingContext;
  */
}
}  // namespace BlackHole
