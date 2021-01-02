#include <metal_stdlib>

using namespace metal;

struct OutVertex {
  float4 position [[position]];
  float4 worldPos;
  float4 normal;
  float2 uv;
  int id [[flat]];
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

struct Mesh {
  device float4 *positions [[id(0)]];
  device float4 *normals [[id(1)]];
  device float2 *uvs [[id(2)]];
  device float4 *tangents [[id(3)]];
  device float2 *lightMapUvs[[id(4)]];
};

struct Material {
  texture2d<float> albedoTex [[id(0)]];
  float4 tintColor [[id(1)]];
  float4 lightMapOff[[id(2)]];
};

vertex OutVertex vertex_project(
        const device Mesh *meshes [[buffer(0)]],
        constant Camera *camera [[buffer(4)]],
        constant float4x4 &modelMatrix [[buffer(5)]],
        constant uint &meshIdx [[buffer(6)]],
        uint vid [[vertex_id]]) {


  OutVertex vertexOut;
  device const Mesh &m = meshes[meshIdx];
  float4 p = m.positions[vid];
  float2 luv = m.lightMapUvs[vid];
  vertexOut.position = camera->VP * (modelMatrix * p);
  vertexOut.worldPos = p;
  vertexOut.normal = modelMatrix * m.normals[vid];
  vertexOut.uv = luv;
  vertexOut.id = meshIdx;
  return vertexOut;
}


fragment half4 fragment_flatcolor(OutVertex vertexIn [[stage_in]],
                                  const device Material *materials [[buffer(0)]]) {

  device const Material &mat = materials[vertexIn.id];
  constexpr sampler s(coord::normalized, filter::linear, address::clamp_to_edge);

float2 uv = vertexIn.uv;
  uv.y = 1.0f - uv.y;
  uv *= mat.lightMapOff.xy;
  uv += mat.lightMapOff.zw;
  float4 albedo =
          mat.albedoTex.sample(s, uv);
  return half4(albedo.x,albedo.y,albedo.z, 1.0h);
}
