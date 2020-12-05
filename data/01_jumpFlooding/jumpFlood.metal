
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

float2 samplePoint(thread float2& minDist, float2 minPos, float2 uvoff,float2 uv, sampler s, texture2d<float>diffuseTexture)
{
    float2 currOffset = uv + uvoff;
    float2 sample = diffuseTexture.sample(s, currOffset).rg;
    float2 delta = sample -uv;
    float dist = dot(delta,delta);
    bool uDiscard= sample.r <= 0.0f;
    bool vDiscard = sample.g <= 0.0f;
    float tt = dot(minDist,minDist);
    float2 currMin = tt < dist ? minDist :delta;
    if(uDiscard && vDiscard)
    {
        return minPos;
    }
    else{
        minDist = currMin;
        return sample;
    }
    //return (uDiscard | vDiscard) > 0?  minDist : currMin;
}

fragment half4 fragment_flatcolor(DataOut data [[stage_in]],
                                  constant Uniforms *uniform [[buffer(0)]],
                                  texture2d<float> diffuseTexture [[texture(0)]])
{
    constexpr sampler s(coord::normalized,
                        address::clamp_to_zero,
                        filter::linear);
    float2 offset = float2(uniform->config.x,uniform->config.x);
    
    
    //float2 offset = float2(512,512);
    float2 size =(float2)uniform->config.yz;
    //float2 size =float2(1904,983);
    //need to convert it to an uv offset
    float2 uvoff = offset/size;

    //sample the gird
    float2 uv = data.uv.xy;
    
    /*
    //top left
    thread float2 minDist = float2(1.0f,1.0f);
    float2 minPos = float2(-1,-1);
    minPos = samplePoint(minDist,minPos, float2(-uvoff.x,uvoff.y),uv,s,diffuseTexture);
    //top
    minPos= samplePoint(minDist,minPos,float2(0,uvoff.y),uv,s,diffuseTexture);
    //top right
    minPos= samplePoint(minDist,minPos,float2(uvoff.x,uvoff.y),uv,s,diffuseTexture);
    //left
    minPos= samplePoint(minDist,minPos,float2(uvoff.x,0),uv,s,diffuseTexture);
    //bottom rigth
    minPos= samplePoint(minDist,minPos,float2(uvoff.x,-uvoff.y),uv,s,diffuseTexture);
    //bottom
    minPos= samplePoint(minDist,minPos,float2(0,-uvoff.y),uv,s,diffuseTexture);
    //bottom left
    minPos= samplePoint(minDist,minPos,float2(-uvoff.x,-uvoff.y),uv,s,diffuseTexture);
    //left
    minPos= samplePoint(minDist,minPos,float2(-uvoff.x,0),uv,s,diffuseTexture);
    //center
    minPos = samplePoint(minDist,minPos,float2(0,0),uv,s,diffuseTexture);
    */
    
    
    float minDist = 99.0f;
    float2 minPos = float2(-1,-1);
    for (int v =-1 ; v <= 1 ; ++v)
    {
        for (int u =-1 ; u <= 1 ; ++u)
        {
            float2 currOffset = uv + uvoff*float2(u,v);
            float2 sample = diffuseTexture.sample(s, currOffset).rg;
            float2 delta = sample -uv;
            float dist = dot(delta,delta);
            bool uDiscard= sample.r <= 0.0f;
            bool vDiscard = sample.g <= 0.0f;
            float currMin = minDist < dist ? minDist :dist;
            if(uDiscard && vDiscard)
            {
                continue;
            }
            minDist = minDist < dist ? minDist :dist;
            minPos = minDist < dist ? minPos:  sample;

        }
        
    }

    return half4(minPos.x,minPos.y,0,1);
}
