
#include <metal_stdlib>

using namespace metal;

constant float4 arrBasePos[6] = {
float4(-1.0f, -1.0f, 0.0f, 1.0f),
float4(1.0f, 1.0f, 1.0f, 0.0f),
float4(-1.0f, 1.0f, 0.0f, 0.0f),
float4(1.0f, 1.0f, 1.0f, 0.0f),
float4(-1.0f, -1.0f, 0.0f, 1.0f),
float4(1.0f, -1.0f, 1.0f, 1.0f)
};

struct DataOut
{
    float4 position [[position]];
    float4 uv;
};

vertex DataOut vertex_project( uint vid [[vertex_id]])
{
    DataOut data;
    float4 vtxData = arrBasePos[vid];
    data.position = float4(vtxData.xy,0.5f,1.0f);
    data.uv.xy = vtxData.zw;
    data.uv.zw =0.0f;
    return data;
}

struct Uniforms
{
    int4 config;
};

fragment half4 fragment_flatcolor(DataOut data [[stage_in]],
                                  constant Uniforms *uniform [[buffer(0)]],
                                  texture2d<float> maskTexture  [[texture(0)]],
                                  texture2d<float> floodTexture [[texture(1)]]
                                  )
{
    float thickness = 0.005*0.005;
    half4 borderColor = half4(1,0.8,0,1);
    half4 noneColor = half4(0,0,0,0);
    
    float w = uniform->config.y;
    float h = uniform->config.z;
    
    float ratio = h/w;

    constexpr sampler s(coord::normalized,
                        address::clamp_to_edge,
                        filter::linear);
    float2 scaledUv = float2(data.uv.x, data.uv.y);
    float mask= maskTexture.sample(s, data.uv.xy).r;
    float2 dist = floodTexture.sample(s, data.uv.xy).rg;

    half4 color = mask > 0.5f ?  noneColor : borderColor;
    float2 delta = (data.uv.xy - dist);
    delta.y *= ratio;
    float distanceFromP = dot(delta,delta);
    color = distanceFromP < thickness ? color :noneColor;

    return color;
}
