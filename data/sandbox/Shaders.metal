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

struct Mesh
{
    device float4 *positions [[id(0)]];
    device float4 *normals [[id(1)]];
    device float2 *uvs [[id(2)]];
    device float4 *tangents [[id(3)]];
    device uint *indices[[id(4)]];
};

vertex OutVertex vertex_project(
                                const device Mesh* meshes[[buffer(0)]],
                                //const device float4 *positions [[buffer(0)]],
                                //const device float4 *normals [[buffer(1)]],
                                //const device float2 *uvs [[buffer(2)]],
                                //const device float4 *tangents [[buffer(3)]],
                                constant Camera *camera [[buffer(4)]],
                                constant float4x4& modelMatrix [[buffer(5)]],
                                constant uint& meshIdx [[buffer(6)]],
                                uint vertexCount [[vertex_id]]) {


  OutVertex vertexOut;
  device const Mesh& m = meshes[meshIdx];
  uint vid = m.indices[vertexCount];
  float4 p = m.positions[vid];
  vertexOut.position = camera->VP * (modelMatrix*p);
  vertexOut.worldPos = p;
  vertexOut.normal = modelMatrix*m.normals[vid];
  vertexOut.uv = m.uvs[vid];
  return vertexOut;
}


fragment half4 fragment_flatcolor(OutVertex vertexIn [[stage_in]],
                                  texture2d<float> rt [[texture(0)]]) {


  float4 n = vertexIn.normal;
  float2 uv = vertexIn.uv;
  return half4(n.x,n.y,n.z,1.0h);
}
