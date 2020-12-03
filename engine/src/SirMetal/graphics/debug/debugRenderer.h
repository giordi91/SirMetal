#pragma once

#include <simd/matrix_types.h>

#include "SirMetal/core/core.h"
#include "SirMetal/core/memory/cpu/stackAllocator.h"
#include <SirMetal/core/memory/gpu/GPUMemoryAllocator.h>
#include "SirMetal/graphics/graphicsDefines.h"
#include "SirMetal/resources/handle.h"
#include "SirMetal/graphics/PSOGenerator.h"
namespace SirMetal {
struct EngineContext;
}

struct BoundingBox;

enum class PRIMITIVE_TYPE { TRIANGLE, LINE, POINT };

namespace SirMetal::graphics {
class CameraController;

class DebugRenderer final {
public:
  DebugRenderer() = default;
  ~DebugRenderer() = default;
  DebugRenderer(const DebugRenderer &) = delete;
  DebugRenderer &operator=(const DebugRenderer &) = delete;
  DebugRenderer(DebugRenderer &&) = delete;
  DebugRenderer &operator=(DebugRenderer &&) = delete;

  void initialize(EngineContext *context);
  void cleanup(EngineContext *context);

  void render(EngineContext *context,
              id commandEncoder,
              SirMetal::graphics::DrawTracker& tracker,
              SirMetal::ConstantBufferHandle cameraBuffer,
              uint32_t renderWidth, uint32_t renderHeight) const;
  void drawAABBs3D(const BoundingBox *data, int count, vector_float4 color);
  void drawLines(const float *data, uint32_t sizeInByte, vector_float4 color);
  void newFrame();

private:
  GPUMemoryAllocator m_gpuAllocator;
  StackAllocator m_allocator;
  StackAllocator m_scratch;
  BufferHandle m_bufferHandle{};
  static constexpr uint64_t SIZE_IN_BYTES = 20 * MB_TO_BYTE;
  static constexpr uint64_t SCRATCH_SIZE_IN_BYTES = 2 * MB_TO_BYTE;
  uint32_t m_linesCount = 0;
  uint32_t m_pointsCount = 0;
  LibraryHandle m_linesShader{};
};

} // namespace SirMetal::graphics
