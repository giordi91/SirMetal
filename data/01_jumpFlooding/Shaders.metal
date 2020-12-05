#include <metal_stdlib>

using namespace metal;

struct Vertex {
  float4 position [[position]];
  float4 normal;
  float2 uv;
};

struct Uniforms {
  float4x4 modelViewProjectionMatrix;
};

vertex Vertex vertex_project(const device float4 *positions [[buffer(0)]],
                             const device float4 *normals [[buffer(1)]],
                             const device float2 *uvs [[buffer(2)]],
                             const device float4 *tangents [[buffer(3)]],
                             constant Uniforms *uniforms [[buffer(4)]],
                             uint vid [[vertex_id]]) {
  Vertex vertexOut;
  vertexOut.position =
      uniforms->modelViewProjectionMatrix * float4(positions[vid]);
  vertexOut.normal = normals[vid];
  vertexOut.uv = uvs[vid];
  return vertexOut;
}

fragment half4 fragment_flatcolor(Vertex vertexIn [[stage_in]]) {
  return half4((half3)(vertexIn.normal.xyz), 1.0f);
}

/*
using namespace metal;

struct Vertex
{
    float4 position [[position]];
    float4 color;
};

vertex Vertex vertex_main(const device Vertex *vertices [[buffer(0)]],
                          uint vid [[vertex_id]])
{
    return vertices[vid];
}

fragment float4 fragment_main(Vertex inVertex [[stage_in]])
{
    return inVertex.color;
}

*/