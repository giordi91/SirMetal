#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;
using namespace raytracing;


constant unsigned int primes[] = {
        2,
        3,
        5,
        7,
        11,
        13,
        17,
        19,
        23,
        29,
        31,
        37,
        41,
        43,
        47,
        53,
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

struct Uniforms {
  unsigned int width;
  unsigned int height;
  unsigned int frameIndex;
  Camera camera;
  AreaLight light;
};

struct Mesh {
  device float4 *positions [[id(0)]];
  device float4 *normals [[id(1)]];
  device float2 *uvs [[id(2)]];
  device float4 *tangents [[id(3)]];
  device uint *indices [[id(4)]];
  texture2d<float> albedoTex [[id(5)]];
  float4x4 matrix [[id(6)]];
  float4 tintColor;
};


kernel void rayKernel(
        instance_acceleration_structure accelerationStructure,
        constant Uniforms &uniforms [[buffer(1)]],
        const device Mesh *meshes [[buffer(2)]],
        texture2d<float, access::write> dstTex [[texture(0)]],
        texture2d<uint> randomTex [[texture(1)]],
        uint2 tid [[thread_position_in_grid]],
        uint2 size [[threads_per_grid]])

{
  typename intersector<triangle_data, instancing>::result_type intersection;

  // Since we aligned the thread count to the threadgroup size, the thread index may be out of bounds
  // of the render target size.
  // Since we aligned the thread count to the threadgroup size, the thread index may be out of bounds
  // of the render target size.
  if (tid.x >= uniforms.width || tid.y >= uniforms.height) {
    return;
  }
  // Compute linear ray index from 2D position

  // Ray we will produce
  ray pray;

  constant Camera &camera = uniforms.camera;

  // Rays start at the camera position
  pray.origin = camera.position;
  pray.min_distance = 0.0f;
  pray.max_distance = INFINITY;

  float2 xy = float2(tid.x + 0.5f, tid.y + 0.5f);// center in the middle of the pixel.
  float2 screenPos = xy / float2(uniforms.width, uniforms.height) * 2.0 - 1.0;

  // Invert Y for DirectX-style coordinates.
  //screenPos.y = -screenPos.y;

  // Unproject the pixel coordinate into a ray.
  float4 world = camera.VPinverse * float4(screenPos, 0, 1);

  world.xyz /= world.w;
  pray.direction = normalize(world.xyz - pray.origin);

  // Create an intersector to test for intersection between the ray and the geometry in the scene.
  intersector<triangle_data, instancing> i;

  //if (!useIntersectionFunctions) {
  i.assume_geometry_type(geometry_type::triangle);
  i.force_opacity(forced_opacity::opaque);
  //}
  i.accept_any_intersection(false);

  intersection = i.intersect(pray, accelerationStructure, 0xFFFFFFFF);
  int instanceIndex = intersection.instance_id;
  uint primitiveIdx = intersection.primitive_id;
  // Stop if the ray didn't hit anything and has bounced out of the scene.

  device const Mesh &m = meshes[instanceIndex];
  uint vid0 = m.indices[primitiveIdx * 3 + 0];
  uint vid1 = m.indices[primitiveIdx * 3 + 1];
  uint vid2 = m.indices[primitiveIdx * 3 + 2];

  float3 a1 = m.normals[vid0].xyz;
  float3 a2 = m.normals[vid1].xyz;
  float3 a3 = m.normals[vid2].xyz;

  float2 bar = intersection.triangle_barycentric_coord;
  float w = 1.0 - bar.x - bar.y;

  float3 outN = normalize(a1 * w + a2 * bar.x + a3 * bar.y);
  outN = (m.matrix * float4(outN.x, outN.y, outN.z, 0.0f)).xyz;

  constexpr float3 skyColor = float3(53 / 255.0f, 81 / 255.0f, 92 / 255.0f);
  float3 outColor = skyColor;
  if (intersection.type != intersection_type::none) {

    outColor = m.tintColor.xyz;
    /*
    a1 = m.positions[vid0].xyz;
    a2 = m.positions[vid1].xyz;
    a3 = m.positions[vid2].xyz;
    float3 outP = normalize(a1 * w + a2 * bar.x + a3 * bar.y);
    outP = (m.matrix * float4(outN.x, outN.y, outN.z, 1.0f)).xyz;

    constexpr int bounces = 1;
    for (int b = 0; b < bounces; ++b) {
      //generate sampling in sphere
      int bounce = b;
      unsigned int offset = randomTex.read(tid).x;
      float2 r = float2(halton(offset + uniforms.frameIndex, 2 + bounce * 4 + 0),
                        halton(offset + uniforms.frameIndex, 2 + bounce * 4 + 1));
      float3 sampleDirection = sampleCosineWeightedHemisphere(r);
      sampleDirection = alignHemisphereWithNormal(sampleDirection, outN);

      ray bRay;
      bRay.origin = outP + outN * 1e-3f;//offsetting to avoid self intersection
      bRay.direction = sampleDirection;
      //ray.direction = outN;
      bRay.min_distance = 0.001f;
      bRay.max_distance = INFINITY;
      intersection = i.intersect(bRay, accelerationStructure, 0xFFFFFFFF);

      float bounceFactor = (1.0f / (bounce + 1.0f));
      if (intersection.type == intersection_type::none) {
        outColor += bounceFactor * skyColor;
      } else {
        instanceIndex = intersection.instance_id;
        primitiveIdx = intersection.primitive_id;
        // Stop if the ray didn't hit anything and has bounced out of the scene.

        device const Mesh &mm = meshes[instanceIndex];

        //outColor += bounceFactor + mm.tintColor.xyz;
       outColor = float3(0.0f,0.0f,0.0f);
}
    }
     */
  }

  dstTex.write(float4(outColor.x, outColor.y, outColor.z, 1.0f), tid);
}
