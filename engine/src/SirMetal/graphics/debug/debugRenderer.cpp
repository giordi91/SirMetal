#include "SirMetal/graphics/debug/debugRenderer.h"

#include "SirMetal/engine.h"
#include "SirMetal/graphics/camera.h"

namespace SirMetal::graphics {

static char const *BUFFER_NAME = "DebugRenderBuffer";

inline int push3ToVec(float *data, const vector_float4 v, int counter) {
  data[counter++] = v.x;
  data[counter++] = v.y;
  data[counter++] = v.z;

  return counter;
}
inline int push3ToVec(float *data, const vector_float3 v, int counter) {
  data[counter++] = v.x;
  data[counter++] = v.y;
  data[counter++] = v.z;

  return counter;
}

inline int push3ToVec(float *data, const vector_float3 v, int counter,
                      const matrix_float4x4 matrix) {
  /*
  vector_float4 modified = matrix * vector_float4{v.x, v.y, v.z, 1.0f};

  data[counter++] = modified.x;
  data[counter++] = modified.y;
  data[counter++] = modified.z;
  */

  return counter;
}
inline int push4ToVec(float *data, const vector_float3 v, int counter) {
  data[counter++] = v.x;
  data[counter++] = v.y;
  data[counter++] = v.z;
  data[counter++] = 1.0f;

  return counter;
}
inline int push3ToVec(float *data, float x, float y, float z, int counter) {
  data[counter++] = x;
  data[counter++] = y;
  data[counter++] = z;

  return counter;
}
inline int push4ToVec(float *data, float x, float y, float z, int counter) {
  data[counter++] = x;
  data[counter++] = y;
  data[counter++] = z;
  data[counter++] = 1.0f;

  return counter;
}
int drawSquareBetweenTwoPoints(float *data, const vector_float3 minP,
                               const vector_float3 maxP, const float y,
                               int counter) {
  counter = push4ToVec(data, minP.x, y, minP.z, counter);
  counter = push4ToVec(data, maxP.x, y, minP.z, counter);

  counter = push4ToVec(data, maxP.x, y, minP.z, counter);
  counter = push4ToVec(data, maxP.x, y, maxP.z, counter);

  counter = push4ToVec(data, maxP.x, y, maxP.z, counter);
  counter = push4ToVec(data, minP.x, y, maxP.z, counter);

  counter = push4ToVec(data, minP.x, y, maxP.z, counter);
  counter = push4ToVec(data, minP.x, y, minP.z, counter);
  return counter;
}
int drawSquareBetweenTwoPointsSize3(float *data, const vector_float3 minP,
                                    const vector_float3 maxP, const float y,
                                    int counter) {
  counter = push3ToVec(data, minP.x, y, minP.z, counter);
  counter = push3ToVec(data, maxP.x, y, minP.z, counter);

  counter = push3ToVec(data, maxP.x, y, minP.z, counter);
  counter = push3ToVec(data, maxP.x, y, maxP.z, counter);

  counter = push3ToVec(data, maxP.x, y, maxP.z, counter);
  counter = push3ToVec(data, minP.x, y, maxP.z, counter);

  counter = push3ToVec(data, minP.x, y, maxP.z, counter);
  counter = push3ToVec(data, minP.x, y, minP.z, counter);
  return counter;
}
int drawSquareBetweenTwoPointsSize3XY(float *data, const vector_float3 minP,
                                      const vector_float3 maxP, const float z,
                                      int counter) {
  counter = push3ToVec(data, minP.x, minP.y, z, counter);
  counter = push3ToVec(data, maxP.x, minP.y, z, counter);

  counter = push3ToVec(data, maxP.x, minP.y, z, counter);
  counter = push3ToVec(data, maxP.x, maxP.y, z, counter);

  counter = push3ToVec(data, maxP.x, maxP.y, z, counter);
  counter = push3ToVec(data, minP.x, maxP.y, z, counter);

  counter = push3ToVec(data, minP.x, maxP.y, z, counter);
  counter = push3ToVec(data, minP.x, minP.y, z, counter);
  return counter;
}

int drawSquareBetweenFourPoints(float *data, const vector_float3 &bottomLeft,
                                const vector_float3 &bottomRight,
                                const vector_float3 &topRight,
                                const vector_float3 &topLeft, int counter) {
  float z = bottomLeft.z;
  counter = push3ToVec(data, bottomLeft.x, bottomLeft.y, z, counter);
  counter = push3ToVec(data, bottomRight.x, bottomRight.y, z, counter);

  counter = push3ToVec(data, bottomRight.x, bottomRight.y, z, counter);
  counter = push3ToVec(data, topRight.x, topRight.y, z, counter);

  counter = push3ToVec(data, topRight.x, topRight.y, z, counter);
  counter = push3ToVec(data, topLeft.x, topLeft.y, z, counter);

  counter = push3ToVec(data, topLeft.x, topLeft.y, z, counter);
  counter = push3ToVec(data, bottomLeft.x, bottomLeft.y, z, counter);
  return counter;
}

void DebugRenderer::initialize(EngineContext *context) {
  /*
  m_allocator.initialize(SIZE_IN_BYTES);
  m_scratch.initialize(SCRATCH_SIZE_IN_BYTES);

  m_bufferHandle = context->m_bufferManager->allocate(
      context->m_renderingContext, SIZE_IN_BYTES, nullptr, sizeof(float) * 8, 0,
      BUFFER_ALLOCATION_BITS::STRUCTURED_BUFFER |
          BUFFER_ALLOCATION_BITS::DYNAMIC_BUFFER,
      BUFFER_NAME);
  ShaderManager *shaderManager = context->m_shaderManager;
  RenderingContext *renderingCtx = context->m_renderingContext;
  m_linesVS = shaderManager->loadShader(
      renderingCtx->getDevice(),
      "../../../engine/src/blackHole/graphics/shaders/colorVS.hlsl",
      SirMetal::SHADER_TYPE::VERTEX);

  m_linesPS = shaderManager->loadShader(
      renderingCtx->getDevice(),
      "../../../engine/src/blackHole/graphics/shaders/colorPS.hlsl",
      SirMetal::SHADER_TYPE::FRAGMENT);
  */
}

void DebugRenderer::cleanup(EngineContext *context) {
  /*
  context->m_bufferManager->free(m_bufferHandle);
  context->m_shaderManager->free(m_linesPS);
  context->m_shaderManager->free(m_linesVS);
  m_bufferHandle = {};
  */
}

void DebugRenderer::render(EngineContext *context,
                           SirMetal::BufferHandle cameraBuffer,
                           uint32_t renderWidth, uint32_t renderHeight) const {
  /*
  if (m_linesCount == 0) return;
  auto *renderingCtx = context->m_renderingContext;
  context->m_bufferManager->update(renderingCtx, m_bufferHandle,
                                   m_allocator.getStartPtr(),
                                   m_linesCount * sizeof(float) * 8);
  ID3D11DeviceContext *deviceContext = renderingCtx->getDeviceContext();
  context->m_shaderManager->bindShader(deviceContext, m_linesVS);
  context->m_shaderManager->bindShader(deviceContext, m_linesPS);

  auto *constantBuffer = context->m_bufferManager->getBuffer(cameraBuffer);
  auto *view = context->m_bufferManager->getSrv(m_bufferHandle);
  deviceContext->VSSetShaderResources(0, 1, &view);
  deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);
  deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
  deviceContext->Draw(m_linesCount, 0);
  */
}

void DebugRenderer::drawAABBs3D(const BoundingBox *data, const int count,
                                const vector_float4 color) {
  /*
  // 12 is the number of lines needed for the AABB, 4 top, 4 bottom, 4
  // vertical two is because we need two points per line, we are not doing
  // line-strip
  const int totalSize = 3 * count * 12 * 2;  // here 3 is the xmfloat4

  auto *points = static_cast<float *>(
      m_scratch.allocate(sizeof(vector_float3) * count * 12 * 2));

  int counter = 0;
  for (int i = 0; i < count; ++i) {
    assert(counter <= totalSize);
    const float3 &minP = data[i].min;
    const float3 &maxP = data[i].max;
    counter =
        drawSquareBetweenTwoPointsSize3(points, minP, maxP, minP.y, counter);
    counter =
        drawSquareBetweenTwoPointsSize3(points, minP, maxP, maxP.y, counter);

    // draw vertical lines
    counter = push3ToVec(points, minP, counter);
    counter = push3ToVec(points, minP.x, maxP.y, minP.z, counter);
    counter = push3ToVec(points, maxP.x, minP.y, minP.z, counter);
    counter = push3ToVec(points, maxP.x, maxP.y, minP.z, counter);

    counter = push3ToVec(points, maxP.x, minP.y, maxP.z, counter);
    counter = push3ToVec(points, maxP.x, maxP.y, maxP.z, counter);

    counter = push3ToVec(points, minP.x, minP.y, maxP.z, counter);
    counter = push3ToVec(points, minP.x, maxP.y, maxP.z, counter);
    assert(counter <= totalSize);
  }
  drawLines(points, totalSize * sizeof(float), color);
  m_scratch.reset();
  */
}

void DebugRenderer::drawLines(const float *data, const uint32_t sizeInByte,
                              const vector_float4 color) {
  // making sure is a multiple of 3, float3 one per point
  assert((sizeInByte % (sizeof(float) * 3) == 0));
  uint32_t count = sizeInByte / (sizeof(float) * 3);

  // TODO we are going to allocate twice the amount of data, since we are going
  // to use a float4s one for position and one for colors, this might be
  // optimized to float3 but needs to be careful
  // https://giordi91.github.io/post/spirvvec3/
  uint32_t finalSize = count * 2 * sizeof(float) * 4;
  auto *paddedData = static_cast<float *>(m_allocator.allocate(finalSize));

  for (uint32_t i = 0; i < count; ++i) {
    paddedData[i * 8 + 0] = data[i * 3 + 0];
    paddedData[i * 8 + 1] = data[i * 3 + 1];
    paddedData[i * 8 + 2] = data[i * 3 + 2];
    paddedData[i * 8 + 3] = 1.0f;

    paddedData[i * 8 + 4] = color.x;
    paddedData[i * 8 + 5] = color.y;
    paddedData[i * 8 + 6] = color.z;
    paddedData[i * 8 + 7] = color.w;
  }
  m_linesCount += count;
}

void DebugRenderer::newFrame() {
  m_allocator.reset();
  m_scratch.reset();
  m_linesCount = 0;
}
}  // namespace SirMetal::graphics
