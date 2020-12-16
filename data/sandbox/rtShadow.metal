#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

constant unsigned int primes[] = {
    2,   3,  5,  7,
    11, 13, 17, 19,
    23, 29, 31, 37,
    41, 43, 47, 53,
};

// Returns the i'th element of the Halton sequence using the d'th prime number as a
// base. The Halton sequence is a "low discrepency" sequence: the values appear
// random but are more evenly distributed then a purely random sequence. Each random
// value used to render the image should use a different independent dimension 'd',
// and each sample (frame) should use a different index 'i'. To decorrelate each
// pixel, a random offset can be applied to 'i'.
float halton(unsigned int i, unsigned int d) {
    unsigned int b = primes[d];

    float f = 1.0f;
    float invB = 1.0f / b;

    float r = 0;

    while (i > 0) {
        f = f * invB;
        r = r + f * (i % b);
        i = i / b;
    }

    return r;
}
// Uses the inversion method to map two uniformly random numbers to a three dimensional
// unit hemisphere where the probability of a given sample is proportional to the cosine
// of the angle between the sample direction and the "up" direction (0, 1, 0)
inline float3 sampleCosineWeightedHemisphere(float2 u) {
    float phi = 2.0f * M_PI_F * u.x;

    float cos_phi;
    float sin_phi = sincos(phi, cos_phi);

    float cos_theta = sqrt(u.y);
    float sin_theta = sqrt(1.0f - cos_theta * cos_theta);

    return float3(sin_theta * cos_phi, cos_theta, sin_theta * sin_phi);
}

// Aligns a direction on the unit hemisphere such that the hemisphere's "up" direction
// (0, 1, 0) maps to the given surface normal direction
inline float3 alignHemisphereWithNormal(float3 sample, float3 normal) {
    // Set the "up" vector to the normal
    float3 up = normal;

    // Find an arbitrary direction perpendicular to the normal. This will become the
    // "right" vector.
    float3 right = normalize(cross(normal, float3(0.0072f, 1.0f, 0.0034f)));

    // Find a third vector perpendicular to the previous two. This will be the
    // "forward" vector.
    float3 forward = cross(right, up);

    // Map the direction on the unit hemisphere to the coordinate system aligned
    // with the normal.
    return sample.x * right + sample.y * up + sample.z * forward;
}


struct Intersection{
    float distance;
    unsigned int primitiveIndex;
    float2 coordinates;
};

struct Ray
{
    packed_float3 origin;
    float minDistance;
    packed_float3 direction;
    float maxDistance;
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

kernel void shadowRayKernel(
                                texture2d<uint> randomTex [[texture(0)]],
                                device const Intersection* intersections [[buffer(0)]],
                                const device float4 *positions [[buffer(1)]],
                                const device float4 *normals [[buffer(2)]],
                                const device float2 *uvs [[buffer(3)]],
                                const device float4 *tangents [[buffer(4)]],
                                const device uint *indeces [[buffer(5)]],
                                device Ray* shadowRays [[buffer(6)]],
                                constant Uniforms & uniforms [[buffer(7)]],
                                uint2 coordinates [[thread_position_in_grid]],
                                uint2 size [[threads_per_grid]],
                                constant unsigned int & bounce [[buffer(8)]]
                                )
{
    uint rayIndex = coordinates.x + coordinates.y * size.x;
    device const Intersection& i = intersections[rayIndex];
    if (i.distance > 0.0f)
    {
        int index = i.primitiveIndex * 3;
        //compute normal
        float3 a1 = normals[indeces[index + 0]].xyz;
        float3 a2 = normals[indeces[index + 1]].xyz;
        float3 a3 = normals[indeces[index + 2]].xyz;

        float w = 1.0 - i.coordinates.x - i.coordinates.y;
        float3 outN = normalize(a1 * i.coordinates.x + a2 * i.coordinates.y + a3 * w);

        //compute pos
        a1 = positions[indeces[index + 0]].xyz;
        a2 = positions[indeces[index + 1]].xyz;
        a3 = positions[indeces[index + 2]].xyz;
        float3 outP = a1 * i.coordinates.x + a2 * i.coordinates.y + a3 * w;

        //generate sampling in sphere
        unsigned int offset = randomTex.read(coordinates).x;
        float2 r = float2(halton(offset + uniforms.frameIndex, 2 + bounce * 4 + 0),
                          halton(offset + uniforms.frameIndex, 2 + bounce * 4 + 1));
        float3 sampleDirection = sampleCosineWeightedHemisphere(r);
        sampleDirection = alignHemisphereWithNormal(sampleDirection, outN);

        device Ray & ray = shadowRays[rayIndex];

        ray.origin =  outP + outN * 1e-3f; //offsetting to avoid self intersection
        ray.direction = sampleDirection;
        //ray.direction = outN;
        ray.minDistance = 0.001f;
        ray.maxDistance = 200.0f;
    }
}
