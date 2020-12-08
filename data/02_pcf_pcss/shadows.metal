#include <metal_stdlib>

using namespace metal;


struct DirLight {
  float4x4 V;
  float4x4 P;
  float4x4 VP;
  float4 lightDir;
};

vertex float4 vertex_project(const device float4 *positions [[buffer(0)]],
                             constant DirLight *light[[buffer(4)]],
                             uint vid [[vertex_id]]) {
  return
      light->VP * positions[vid];
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