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
  float lightSize;
  float near;
  float pcfsize;
  int pcfsamples;
  int blockerCount;
  int showBlocker;
  int algType;
};

constant float2 poissonDisk[64] = {
    float2(0.0617981, 0.07294159),  float2(0.6470215, 0.7474022),
    float2(-0.5987766, -0.7512833), float2(-0.693034, 0.6913887),
    float2(0.6987045, -0.6843052),  float2(-0.9402866, 0.04474335),
    float2(0.8934509, 0.07369385),  float2(0.1592735, -0.9686295),
    float2(-0.05664673, 0.995282),  float2(-0.1203411, -0.1301079),
    float2(0.1741608, -0.1682285),  float2(-0.09369049, 0.3196758),
    float2(0.185363, 0.3213367),    float2(-0.1493771, -0.3147511),
    float2(0.4452095, 0.2580113),   float2(-0.1080467, -0.5329178),
    float2(0.1604507, 0.5460774),   float2(-0.4037193, -0.2611179),
    float2(0.5947998, -0.2146744),  float2(0.3276062, 0.9244621),
    float2(-0.6518704, -0.2503952), float2(-0.3580975, 0.2806469),
    float2(0.8587891, 0.4838005),   float2(-0.1596546, -0.8791054),
    float2(-0.3096867, 0.5588146),  float2(-0.5128918, 0.1448544),
    float2(0.8581337, -0.424046),   float2(0.1562584, -0.5610626),
    float2(-0.7647934, 0.2709858),  float2(-0.3090832, 0.9020988),
    float2(0.3935608, 0.4609676),   float2(0.3929337, -0.5010948),
    float2(-0.8682281, -0.1990303), float2(-0.01973724, 0.6478714),
    float2(-0.3897587, -0.4665619), float2(-0.7416366, -0.4377831),
    float2(-0.5523247, 0.4272514),  float2(-0.5325066, 0.8410385),
    float2(0.3085465, -0.7842533),  float2(0.8400612, -0.200119),
    float2(0.6632416, 0.3067062),   float2(-0.4462856, -0.04265022),
    float2(0.06892014, 0.812484),   float2(0.5149567, -0.7502338),
    float2(0.6464897, -0.4666451),  float2(-0.159861, 0.1038342),
    float2(0.6455986, 0.04419327),  float2(-0.7445076, 0.5035095),
    float2(0.9430245, 0.3139912),   float2(0.0349884, -0.7968109),
    float2(-0.9517487, 0.2963554),  float2(-0.7304786, -0.01006928),
    float2(-0.5862702, -0.5531025), float2(0.3029106, 0.09497032),
    float2(0.09025345, -0.3503742), float2(0.4356628, -0.0710125),
    float2(0.4112572, 0.7500054),   float2(0.3401214, -0.3047142),
    float2(-0.2192158, -0.6911137), float2(-0.4676369, 0.6570358),
    float2(0.6295372, 0.5629555),   float2(0.1253822, 0.9892166),
    float2(-0.1154335, 0.8248222),  float2(-0.4230408, -0.7129914),
};

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

#define PCF_NUM_SAMPLES 16
#define NEAR_PLANE 0.02
#define LIGHT_WORLD_SIZE .5

float2 FindBlocker(float2 uv, float lightSizeUV, float zReceiver, float zVS,
                   depth2d<float> shadowMap, float near, int blockerCount) {

  constexpr sampler s(coord::normalized, filter::nearest,
                      address::clamp_to_edge, compare_func::less,
                      lod_clamp(0.0f, 0.0f));
  float2 result;
  // This uses similar triangles to compute what
  // area of the shadow map we should search
  float searchWidth = lightSizeUV * saturate(zReceiver - near) / zReceiver;
  float blockerSum = 0;
  result.y = 0;
  for (int i = 0; i < blockerCount; ++i) {

    float shadowMapDepth =
        shadowMap.sample(s, uv + poissonDisk[i] * searchWidth);
    if (shadowMapDepth < (zReceiver - 0.00004f)) {
      blockerSum += shadowMapDepth;
      result.y += 1.0f;
    }
  }
  result.x = blockerSum / result.y;
  return result;
}

float PCF_Filter(float2 uv, float zReceiver, float filterRadiusUV,
                 depth2d<float> shadowMap, int sampleCount) {
  constexpr sampler s(coord::normalized, filter::linear, address::clamp_to_edge,
                      compare_func::less, lod_clamp(0.0f, 0.0f));
  float sum = 0.0f;
  for (int i = 0; i < sampleCount; ++i) {
    float2 offset = poissonDisk[i] * filterRadiusUV;
    sum += shadowMap.sample_compare(s, uv + offset, zReceiver);
  }
  return sum / sampleCount;
}

fragment half4 fragment_flatcolor(OutVertex vertexIn [[stage_in]],
                                  constant DirLight *light [[buffer(5)]],
                                  depth2d<float> shadowMap [[texture(0)]]) {

  float4 wp = vertexIn.worldPos;
  wp.w = 1.0f;
  float4 shadowPos = light->VP * wp;
  shadowPos /= shadowPos.w;

  // convert to light projected space
  // we only remap xy coords to range [0,1] from [-1,1] since the z is already
  // in that range
  float2 xy = shadowPos.xy;
  xy = xy * 0.5 + 0.5;
  xy.y = 1 - xy.y;

  float lightFactor = 1.0f;
  if (light->algType == 1) {

    // now we get also the position in light space, no projection
    float4 shadowVS = light->V * wp;
    float zVS = (shadowVS.z / shadowVS.w);

    // find blockers
    float2 q = FindBlocker(xy, light->lightSize, shadowPos.z, zVS, shadowMap,
                           light->near, light->blockerCount);
    if (light->showBlocker) {
      float v = q.y / light->blockerCount;
      return half4(v, v, v, 1.0f);
    }

    float distance = q.y > 1 ? q.x : -1;
    if (distance > 0) {
      constexpr sampler s(coord::normalized, filter::linear,
                          address::clamp_to_edge, compare_func::less,
                          lod_clamp(0.0f, 0.0f));

      float w = light->lightSize * (shadowPos.z - distance) / distance;

      lightFactor =
          PCF_Filter(xy, shadowPos.z - 0.00005f, w * 80 * light->pcfsize,
                     shadowMap, light->pcfsamples);
    }
  } else {
    // Working pcf
    lightFactor = PCF_Filter(xy, shadowPos.z - 0.00005f, light->pcfsize,
                             shadowMap, light->pcfsamples);
  }

  float d = saturate(
      dot(normalize(light->lightDir.xyz), normalize(vertexIn.normal.xyz)));
  d *= lightFactor;

  return half4(d, d, d, 1.0f);
}
