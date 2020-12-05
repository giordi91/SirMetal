
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

fragment half4 fragment_flatcolor(DataOut data [[stage_in]],
                                  texture2d<float> diffuseTexture [[texture(0)]])
{
    constexpr sampler s(coord::normalized,
                        address::repeat,
                        filter::linear);
    float mask= diffuseTexture.sample(s, data.uv.xy).r;
//    half2 color = mask > 0.5f ? half2(data.uv.x,data.uv.y) : half2(0,0);
    half2 color = mask > 0.5f ? half2(data.uv.x,data.uv.y) : half2(-1,-1);
    return half4(color,0,1.0f);
}
