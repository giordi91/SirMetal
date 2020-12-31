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
  device float2 *lightMapUvs[[id(4)]];
};

struct Material {
  texture2d<float> albedoTex [[id(0)]];
  sampler sampler [[id(1)]];
  float4 tintColor [[id(2)]];
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
                                  constant DirLight *light [[buffer(5)]],
                                  const device Material *materials [[buffer(0)]]) {

  device const Material &mat = materials[vertexIn.id];
  float4 n = vertexIn.normal;

  float2 uv = vertexIn.uv;
  uv.y = 1.0f - uv.y;
  float4 albedo =
          mat.albedoTex.sample(mat.sampler, uv);
  float4 color = mat.tintColor * albedo;
  color*=  saturate(dot(n.xyz,light->lightDir.xyz));
  if(vertexIn.id == 1){
    color = albedo;
  }

  return half4(color.x,color.y,color.z, 1.0h);
}
