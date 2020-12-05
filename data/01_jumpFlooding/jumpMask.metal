

#include <metal_stdlib>

using namespace metal;

struct Vertex
{
    float4 position [[position]];
};

struct Uniforms
{
    float4x4 modelViewProjectionMatrix;
};


vertex Vertex vertex_project(const device float4 *positions [[buffer(0)]],
                             constant Uniforms *uniforms   [[buffer(1)]],
                             uint vid [[vertex_id]])
{
    Vertex vertexOut;
    vertexOut.position = uniforms->modelViewProjectionMatrix * positions[vid];
    return vertexOut;
}

fragment half4 fragment_flatcolor(Vertex vertexIn [[stage_in]])
{
    return half4(1,1,1,1.0f);
}

