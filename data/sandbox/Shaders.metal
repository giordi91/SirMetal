#include <metal_stdlib>

using namespace metal;

struct OutVertex {
  float4 position [[position]];
  float4 worldPos;
  float4 normal;
  float2 uv;
};

struct Camera {
  float4x4 viewMatrix;
  float4x4 viewInverse;
  float4x4 projection;
  float4x4 VP;
  float4x4 VPInverse;
  float screenWidth;
  float screenHeight;
  float nearPlane;
  float farPlane;
  float fov;
};

struct DirLight {
  float4x4 V;
  float4x4 P;
  float4x4 VP;
  float4 lightDir;
};

constant float2 poissonDisk[16] = {
    float2(-0.94201624, -0.39906216),  float2(0.94558609, -0.76890725),
    float2(-0.094184101, -0.92938870), float2(0.34495938, 0.29387760),
    float2(-0.91588581, 0.45771432),   float2(-0.81544232, -0.87912464),
    float2(-0.38277543, 0.27676845),   float2(0.97484398, 0.75648379),
    float2(0.44323325, -0.97511554),   float2(0.53742981, -0.47373420),
    float2(-0.26496911, -0.41893023),  float2(0.79197514, 0.19090188),
    float2(-0.24188840, 0.99706507),   float2(-0.81409955, 0.91437590),
    float2(0.19984126, 0.78641367),    float2(0.14383161, -0.14100790)};

vertex OutVertex vertex_project(const device float4 *positions [[buffer(0)]],
                                const device float4 *normals [[buffer(1)]],
                                const device float2 *uvs [[buffer(2)]],
                                const device float4 *tangents [[buffer(3)]],
                                constant Camera *camera [[buffer(4)]],
                                uint vid [[vertex_id]]) {
  OutVertex vertexOut;
  vertexOut.position = camera->VP * positions[vid];
  vertexOut.worldPos = positions[vid];
  vertexOut.normal = normals[vid];
  vertexOut.uv = uvs[vid];
  return vertexOut;
}

#define BLOCKER_SEARCH_NUM_SAMPLES 16
#define PCF_NUM_SAMPLES 16
#define NEAR_PLANE 0.02
#define LIGHT_WORLD_SIZE .5
#define LIGHT_FRUSTUM_WIDTH 3.75
// Assuming that LIGHT_FRUSTUM_WIDTH == LIGHT_FRUSTUM_HEIGHT
//#define LIGHT_SIZE_UV (LIGHT_WORLD_SIZE / LIGHT_FRUSTUM_WIDTH)
#define LIGHT_SIZE_UV 0.5f

float2 FindBlocker(float2 uv, float lightSizeUV ,float zReceiver, depth2d<float> shadowMap) {

  constexpr sampler s(coord::normalized, filter::nearest,
                      address::clamp_to_edge, compare_func::less,
                      lod_clamp(0.0f, 0.0f));
  float2 result;
  // This uses similar triangles to compute what
  // area of the shadow map we should search
  float searchWidth = LIGHT_SIZE_UV * saturate(zReceiver - 0.01) / zReceiver;
  float blockerSum = 0;
  result.y = 0;
  for (int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; ++i) {

    float shadowMapDepth = shadowMap.sample(s, uv + poissonDisk[i] * 0.026);
    // shadowMap.sample(s, uv + poissonDisk[i] * 0.015);
    // float shadowMapDepth = tDepthMap.SampleLevel(
    //    PointSampler, uv + poissonDisk[i] * searchWidth, 0);
    if (shadowMapDepth < (zReceiver - 0.00005f)) {
      blockerSum += shadowMapDepth;
      result.y += 1.0f;
    }
  }
  result.x = blockerSum / result.y;
  return result;
}
float PenumbraSize(float zReceiver, float zBlocker) // Parallel plane estimation
{
  return (saturate(zReceiver - zBlocker) * 0.026) / zBlocker;
}

float PCF_Filter(float2 uv, float zReceiver, float filterRadiusUV,
                 depth2d<float> shadowMap) {

  constexpr sampler s(coord::normalized, filter::linear, address::clamp_to_edge,
                      compare_func::less, lod_clamp(0.0f, 0.0f));
  float sum = 0.0f;
  for (int i = 0; i < PCF_NUM_SAMPLES; ++i) {
    float2 offset = poissonDisk[i] * filterRadiusUV;
    // sum += tDepthMap.sample_compare(PCF_Sampler, uv + offset, zReceiver);
    sum += shadowMap.sample_compare(s, uv + offset, zReceiver - 0.00005f);
  }
  return sum / PCF_NUM_SAMPLES;
}

float SearchWidth(float uvLightSize, float receiverDistance) {
  return uvLightSize * saturate(receiverDistance - 0.01) / receiverDistance;
}
float FindBlockerDistance(float3 shadowCoords, depth2d<float> shadowMap,
                          float uvLightSize) {
  constexpr sampler s(coord::normalized, filter::nearest,
                      address::clamp_to_edge, compare_func::less,
                      lod_clamp(0.0f, 0.0f));
  int blockers = 0;
  float avgBlockerDistance = 0;
  float searchWidth = SearchWidth(uvLightSize, shadowCoords.z);
  for (int i = 0; i < 16; i++) {
    float z =
        shadowMap.sample(s, shadowCoords.xy + poissonDisk[i] * searchWidth);
    if (z < (shadowCoords.z - 0.00005f)) {
      blockers++;
      avgBlockerDistance += z;
    }
  }
  if (blockers > 0)
    return avgBlockerDistance / blockers;
  else
    return -1;
}

fragment half4 fragment_flatcolor(OutVertex vertexIn [[stage_in]],
                                  constant DirLight *light [[buffer(5)]],
                                  depth2d<float> shadowMap [[texture(0)]]) {

  float4 wp = vertexIn.worldPos;
  float4 shadowPos = light->VP * wp;
  shadowPos /= shadowPos.w;
  float2 xy = shadowPos.xy;
  xy = xy * 0.5 + 0.5;
  xy.y = 1 - xy.y;

// find blockers
  float2  q = FindBlocker(xy,216.0f, shadowPos.z, shadowMap);
  //float distance = FindBlockerDistance(shadowPos.xyz, shadowMap, 2.0f);
  float distance = q.y > 1 ? q.x : -1;
  float lightFactor= 1.0f;
  if (distance > 0) {
    constexpr sampler s(coord::normalized, filter::linear,
                        address::clamp_to_edge, compare_func::less,
                        lod_clamp(0.0f, 0.0f));
    //    float shadowMapDepth = shadowMap.sample(s, xy);
    float shadowMapDepth = distance;
    float w = (shadowPos.z - shadowMapDepth) / shadowMapDepth;
    const int neighborWidth = 6;
    const float neighbors =
        (neighborWidth * 2.0 + 1.0) * (neighborWidth * 2.0 + 1.0);
    float mapSize = 2048;
    float texelSize = 1.0 / mapSize;
    float total = 0.0;
    for (int x = -neighborWidth; x <= neighborWidth; x++) {
      for (int y = -neighborWidth; y <= neighborWidth; y++) {
        float shadow_sample = shadowMap.sample_compare(
            s, xy + float2(x, y) * w*2, shadowPos.z - 0.00005f);
        // float current_sample = shadowPos.z - 0.00005f;
        // if (current_sample > shadow_sample) {
        total += shadow_sample;
        //}
      }
    }
    total /= neighbors;
    lightFactor = (total);
  }
  // return half4(1,0,0,1);

  /*
  if (q.y > 1) {
    float penumbraRatio = PenumbraSize(shadowPos.z, q.x);
    float filterRadiusUV =
        penumbraRatio * LIGHT_SIZE_UV; //* NEAR_PLANE / shadowPos.z;

  //return half4(penumbraRatio,0,0,1);


    //lightFactor = PCF_Filter(xy, shadowPos.z, 0.005,shadowMap);
    // pcf
    //lightFactor = 0.0f;

    constexpr sampler s(coord::normalized, filter::linear,
                        address::clamp_to_edge, compare_func::less,
                        lod_clamp(0.0f, 0.0f));
    const int neighborWidth = 3;
    const float neighbors =
        (neighborWidth * 2.0 + 1.0) * (neighborWidth * 2.0 + 1.0);
    float mapSize = 2048;
    float texelSize = 1.0 / mapSize;
    float total = 0.0;
    for (int x = -neighborWidth; x <= neighborWidth; x++) {
      for (int y = -neighborWidth; y <= neighborWidth; y++) {
        float shadow_sample = shadowMap.sample_compare(
            s, xy + float2(x, y) * texelSize, shadowPos.z - 0.00005f);
        // float current_sample = shadowPos.z - 0.00005f;
        // if (current_sample > shadow_sample) {
        total += shadow_sample;
        //}
      }
    }
    total /= neighbors;
    lightFactor = (total);
    //return half4(1,0,0,1);
  }
   */

  // lightFactor = LIGHT_SIZE_UV * saturate(shadowPos.z- NEAR_PLANE) /
  // shadowPos.z;

  float d = saturate(
      dot(normalize(light->lightDir.xyz), normalize(vertexIn.normal.xyz)));
  d *= lightFactor;

  return half4(d, d, d, 1.0f);
}

/*
using namespace metal;

struct Vertex
{
    float4 position [[position]];
    float4 color;
};

vertex Vertex vertex_main(const device Vertex *vertices [[buffer(0)]],
                          uint vid [[vertex_id]])
{
    return vertices[vid];
}

fragment float4 fragment_main(Vertex inVertex [[stage_in]])
{
    return inVertex.color;
}

*/
/*
float sample = shadowMap.sample(s, xy);
float inShadow = (shadowPos.z - 0.0005f) < sample ? 1.0f : 0.1f;
float d = saturate(dot(light->lightDir.xyz, vertexIn.normal.xyz));
d *= inShadow;
*/
