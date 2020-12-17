#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;
struct Camera {
    vector_float3 position;
    vector_float3 right;
    vector_float3 up;
    vector_float3 forward;
};

struct AreaLight {
    vector_float3 position;
    vector_float3 forward;
    vector_float3 right;
    vector_float3 up;
    vector_float3 color;
};

struct Uniforms
{
    unsigned int width;
    unsigned int height;
    unsigned int frameIndex;
    Camera camera;
    AreaLight light;
};


struct Intersection{
    float distance;
    unsigned int primitiveIndex;
    float2 coordinates;
};

kernel void shadeKernel(
                                texture2d<float, access::write> image [[texture(0)]],
                                texture2d<float> prevImage [[texture(1)]],
                                device const Intersection* intersections [[buffer(0)]],
                                const device float4 *positions [[buffer(1)]],
                                const device float4 *normals [[buffer(2)]],
                                const device float2 *uvs [[buffer(3)]],
                                const device float4 *tangents [[buffer(4)]],
                                const device uint *indeces [[buffer(5)]],
                                constant Uniforms & uniforms [[buffer(6)]],
                                uint2 coordinates [[thread_position_in_grid]],
                                uint2 size [[threads_per_grid]])
{
    uint rayIndex = coordinates.x + coordinates.y * size.x;
    device const Intersection& i = intersections[rayIndex];
    float3 skyColor = float3(53/255.0f,81/255.0f,92/255.0f);
    float3 outColor;

    if (i.distance > 0.0f)
    {
        outColor = float3(0,0,0);
    }
    else{
        outColor = skyColor;
    }

    //here we perform a cumulative moving average
    //we skip the first frame because the previous texture would be empty
    if(uniforms.frameIndex > 1){
        float3 color = prevImage.read(coordinates).xyz;
        color *= uniforms.frameIndex;
        color +=outColor;
        color /= (uniforms.frameIndex + 1);
        outColor = saturate(color);
    }

    //writing the final color
    image.write(float4(outColor, 1.0), coordinates);


}