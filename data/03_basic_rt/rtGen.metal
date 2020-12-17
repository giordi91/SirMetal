#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Camera {
    float4x4 VPinverse;
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


struct Ray
{
    packed_float3 origin;
    float minDistance;
    packed_float3 direction;
    float maxDistance;
};

kernel void rayKernel(device Ray* rays [[buffer(0)]],
                        constant Uniforms & uniforms [[buffer(1)]],
                         uint2 tid  [[thread_position_in_grid]],
                         uint2 size [[threads_per_grid]]
                         )
{
    // Since we aligned the thread count to the threadgroup size, the thread index may be out of bounds
    // of the render target size.
    if (tid.x < uniforms.width && tid.y < uniforms.height) {
        // Compute linear ray index from 2D position
        uint rayIdx = tid.y * uniforms.width + tid.x;

        // Ray we will produce
        device Ray & ray = rays[rayIdx];

        constant Camera & camera = uniforms.camera;

        // Rays start at the camera position
        ray.origin = camera.position;
        ray.minDistance = 0.0f;
        ray.maxDistance = 200.0f;

        float2 xy = float2(tid.x + 0.5f, tid.y + 0.5f); // center in the middle of the pixel.
        float2 screenPos = xy / float2(uniforms.width, uniforms.height) * 2.0 - 1.0;

        // Unproject the pixel coordinate into a ray.
        float4 world = camera.VPinverse *float4(screenPos, 0, 1);

        world.xyz /= world.w;
        ray.direction = normalize(world.xyz - ray.origin);
    }
}