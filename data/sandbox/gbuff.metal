#include <metal_stdlib>

using namespace metal;

struct OutVertex {
  float4 position [[position]];
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
  device float2 *lightMapUvs [[id(4)]];
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
  return vertexOut;
}


struct FragmentOut {
  uint position [[color(0)]];
  float2 uv [[color(1)]];
};
fragment FragmentOut fragment_flatcolor(OutVertex vertexIn [[stage_in]],
                                        const float3 bary [[barycentric_coord]],
                                        const uint pid [[primitive_id]]
                                        ) {

  FragmentOut fout;
  fout.position = pid;
  fout.uv = bary.xy;
  return fout;
}
