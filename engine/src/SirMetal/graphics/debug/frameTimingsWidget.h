#pragma once
#include <stdint.h>

namespace SirMetal{
struct EngineContext;
}

namespace SirMetal::graphics {
struct FrameTimingsWidget final {
  static const int NUMBER_OF_SAMPLES = 200;
  static const int NUMBER_OF_HISTOGRAMS_BUCKETS = 11;
#define MAX_FRAME_TIME 20.0f
  FrameTimingsWidget();
  void render(EngineContext *context);
  float m_samples[NUMBER_OF_SAMPLES];
  float m_tempSamples[NUMBER_OF_SAMPLES];
  uint32_t m_runningCounter = 0;
  float m_framesHistogram[NUMBER_OF_HISTOGRAMS_BUCKETS]{};
};
} // namespace SirMetal::graphics
