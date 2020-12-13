#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;
/*
struct Ray {
    // Starting point
    packed_float3 origin;
    // Direction the ray is traveling
    packed_float3 direction;
};

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

// Represents an intersection between a ray and the scene, returned by the MPSRayIntersector.
// The intersection type is customized using properties of the MPSRayIntersector.
struct Intersection {
    // The distance from the ray origin to the intersection point. Negative if the ray did not
    // intersect the scene.
    float distance;
    
    // The index of the intersected primitive (triangle), if any. Undefined if the ray did not
    // intersect the scene.
    int primitiveIndex;
    
    // The barycentric coordinates of the intersection point, if any. Undefined if the ray did
    // not intersect the scene.
    float2 coordinates;
};

constant unsigned int primes[] = {
    2,   3,  5,  7,
    11, 13, 17, 19,
    23, 29, 31, 37,
    41, 43, 47, 53,
};

// Consumes ray/triangle intersection results to compute the shaded image
kernel void shadeKernel(uint2 tid [[thread_position_in_grid]],
                        constant Uniforms & uniforms[[buffer(0)]],
                        device Ray *rays [[buffer(1)]],
                        device Intersection *intersections [[buffer(2)]],
                        texture2d<float, access::write> dstTex [[texture(0)]] )
{
    if (tid.x < uniforms.width && tid.y < uniforms.height) {
        unsigned int rayIdx = tid.y * uniforms.width + tid.x;
        device Intersection & intersection = intersections[rayIdx];


        // Intersection distance will be negative if ray missed or was disabled in a previous
        // iteration.
        if ( intersection.distance >= 0.0f) {
                dstTex.write(float4(1,0,0, 1.0f), tid);
        }
        else {
                // In this case, a ray coming from the camera hit the light source directly, so
                // we'll write the light color into the output image.
                dstTex.write(float4(0,0,1, 1.0f), tid);
        }
    }
}
*/


struct Intersection{
    float distance;
    unsigned int primitiveIndex;
    float2 coordinates;
};

kernel void shadeKernel(texture2d<float, access::write> image [[texture(0)]],
                                device const Intersection* intersections [[buffer(0)]],
                                const device float4 *positions [[buffer(1)]],
                                const device float4 *normals [[buffer(2)]],
                                const device float2 *uvs [[buffer(3)]],
                                const device float4 *tangents [[buffer(4)]],
                                const device uint *indeces [[buffer(5)]],
                                uint2 coordinates [[thread_position_in_grid]],
                                uint2 size [[threads_per_grid]])
{
    uint rayIndex = coordinates.x + coordinates.y * size.x;
    device const Intersection& i = intersections[rayIndex];
    if (i.distance > 0.0f)
    {
        int index = i.primitiveIndex * 3;
        float3 n1 = normals[indeces[index + 0]].xyz;
        float3 n2 = normals[indeces[index + 1]].xyz;
        float3 n3 = normals[indeces[index + 2]].xyz;

        float w = 1.0 - i.coordinates.x - i.coordinates.y;
        float3 outN = n1 * i.coordinates.x + n2 * i.coordinates.y + n3 * w;
        image.write(float4(outN, 1.0), coordinates);
        //image.write(float4(i.coordinates, w, 1.0), coordinates);
    }
}