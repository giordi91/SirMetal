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
*/

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

/*
// Generates rays starting from the camera origin and traveling towards the image plane aligned
// with the camera's coordinate system.
kernel void rayKernel(uint2 tid [[thread_position_in_grid]],
                      // Buffers bound on the CPU. Note that 'constant' should be used for small
                      // read-only data which will be reused across threads. 'device' should be
                      // used for writable data or data which will only be used by a single thread.
                      constant Uniforms & uniforms [[buffer(0)]],
                      device Ray *rays [[buffer(1)]])
{
    // Since we aligned the thread count to the threadgroup size, the thread index may be out of bounds
    // of the render target size.
    if (tid.x < uniforms.width && tid.y < uniforms.height) {
        // Compute linear ray index from 2D position
        uint rayIdx = tid.y * uniforms.width + tid.x;

        // Ray we will produce
        device Ray & ray = rays[rayIdx];

        // Pixel coordinates for this thread
        float2 pixel = (float2)tid;

        // Apply a random offset to random number index to decorrelate pixels
        //unsigned int offset = randomTex.read(tid).x;
        
        //// Add a random offset to the pixel coordinates for antialiasing
        //float2 r = float2(halton(offset + uniforms.frameIndex, 0),
        //                  halton(offset + uniforms.frameIndex, 1));
        // Add a random offset to the pixel coordinates for antialiasing
        //pixel += r;
        
        // Map pixel coordinates to -1..1
        float2 uv = (float2)pixel / float2(uniforms.width, uniforms.height);
        uv = uv * 2.0f - 1.0f;
        
        constant Camera & camera = uniforms.camera;
        
        // Rays start at the camera position
        ray.origin = camera.position;
        
        // Map normalized pixel coordinates into camera's coordinate system
        ray.direction = normalize(uv.x * camera.right +
                                  uv.y * camera.up +
                                  camera.forward);
    }
}
*/

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
    /*
    uint rayIndex = coordinates.x + coordinates.y * size.x;
    float2 uv = float2(coordinates) / float2(size - 1);
    rays[rayIndex].origin = packed_float3(uv.x, uv.y, -1.0);
    rays[rayIndex].direction = packed_float3(0.0, 0.0, 1.0);
    rays[rayIndex].minDistance = 0.0f;
    rays[rayIndex].maxDistance = 2.0f;
    */

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

        // Invert Y for DirectX-style coordinates.
        //screenPos.y = -screenPos.y;

        // Unproject the pixel coordinate into a ray.
        float4 world = camera.VPinverse *float4(screenPos, 0, 1);

        world.xyz /= world.w;
        ray.direction = normalize(world.xyz - ray.origin);
    }
}