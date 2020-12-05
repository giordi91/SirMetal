#include <metal_stdlib>

using namespace metal;

struct Vertex {
  float4 position [[position]];
  float4 color;
};

struct Camera {
  float4x4 viewMatrix;
  float4x4 viewInverse;
  float4x4 projection;
  float4x4 VP;
  float4x4 VPInverse;
  float screenWidth;
  float screenHeight;
  float nearPlane;
  float farPlane;
  float fov;
};

vertex Vertex vertex_project(const device Vertex *data [[buffer(0)]],
                             constant Camera *camera [[buffer(4)]],
                             uint vid [[vertex_id]]) {
  Vertex vertexOut;
  vertexOut.position =
      camera->VP* float4(data[vid].position);
  vertexOut.color = data[vid].color;
  return vertexOut;
}

fragment half4 fragment_flatcolor(Vertex vertexIn [[stage_in]]) {
  return half4(vertexIn.color);
}
