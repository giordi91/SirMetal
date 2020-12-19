#include <metal_stdlib>

using namespace metal;

struct OutVertex {
  float4 position [[position]];
  float4 worldPos;
  float4 normal;
  float2 uv;
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


vertex OutVertex vertex_project(
                                const device float4 *positions [[buffer(0)]],
                                const device float4 *normals [[buffer(1)]],
                                const device float2 *uvs [[buffer(2)]],
                                const device float4 *tangents [[buffer(3)]],
                                constant Camera *camera [[buffer(4)]],
                                constant float4x4& modelMatrix [[buffer(5)]],
                                uint vid [[vertex_id]]) {
  OutVertex vertexOut;
  vertexOut.position = camera->VP * (modelMatrix*positions[vid]);
  vertexOut.worldPos = positions[vid];
  vertexOut.normal = modelMatrix*normals[vid];
  vertexOut.uv = uvs[vid];
  return vertexOut;
}


fragment half4 fragment_flatcolor(OutVertex vertexIn [[stage_in]],
                                  texture2d<float> rt [[texture(0)]]) {


  float4 n = vertexIn.normal;
  float2 uv = vertexIn.uv;
  return half4(n.x,n.y,n.z,1.0h);
}
