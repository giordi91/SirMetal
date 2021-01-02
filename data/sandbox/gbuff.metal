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

struct DirLight {
  float4x4 V;
  float4x4 P;
  float4x4 VP;
  float4 lightDir;
};

struct Mesh {
  device float4 *positions [[id(0)]];
  device float4 *normals [[id(1)]];
  device float2 *uvs [[id(2)]];
  device float4 *tangents [[id(3)]];
  device float2 *lightMapUvs [[id(4)]];
};

struct Material {
  texture2d<float> albedoTex [[id(0)]];
  float4 tintColor [[id(1)]];
  float4 lightMapOff[[id(2)]];
};

vertex OutVertex vertex_project(const device Mesh *meshes [[buffer(0)]],
                                constant Camera *camera [[buffer(4)]],
                                constant float4x4 &modelMatrix [[buffer(5)]],
                                constant uint &meshIdx [[buffer(6)]],
                                constant float2 &jitter[[buffer(7)]],
                                uint vid [[vertex_id]]) {


  OutVertex vertexOut;
  device const Mesh &m = meshes[meshIdx];
  //export in uv space
  float2 luv = m.lightMapUvs[vid];
  vertexOut.position = float4((luv  * 2 - 1.0f), 0.5f, 1);
  vertexOut.position.xy += (jitter);

  float4 p = m.positions[vid];
  vertexOut.worldPos = modelMatrix * p;
  vertexOut.normal = modelMatrix * m.normals[vid];
  vertexOut.uv = luv;
  return vertexOut;
}


struct FragmentOut {
  float4 position [[color(0)]];
  float2 uv [[color(1)]];
  float4 normal [[color(2)]];
};
fragment FragmentOut fragment_flatcolor(OutVertex vertexIn [[stage_in]],
                                        const device Material *materials [[buffer(0)]]) {

  FragmentOut fout;
  fout.position = vertexIn.worldPos;
  fout.uv = vertexIn.uv;
  fout.normal = normalize(vertexIn.normal) * 0.5f + 0.5f;
  return fout;
}
