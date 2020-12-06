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

/*
constant float2 poissonDisk[128] ={
    float2(-0.9406119, 0.2160107),
    float2(-0.920003, 0.03135762),
    float2(-0.917876, -0.2841548),
    float2(-0.9166079, -0.1372365),
    float2(-0.8978907, -0.4213504),
    float2(-0.8467999, 0.5201505),
    float2(-0.8261013, 0.3743192),
    float2(-0.7835162, 0.01432008),
    float2(-0.779963, 0.2161933),
    float2(-0.7719588, 0.6335353),
    float2(-0.7658782, -0.3316436),
    float2(-0.7341912, -0.5430729),
    float2(-0.6825727, -0.1883408),
    float2(-0.6777467, 0.3313724),
    float2(-0.662191, 0.5155144),
    float2(-0.6569989, -0.7000636),
    float2(-0.6021447, 0.7923283),
    float2(-0.5980815, -0.5529259),
    float2(-0.5867089, 0.09857152),
    float2(-0.5774597, -0.8154474),
    float2(-0.5767041, -0.2656419),
    float2(-0.575091, -0.4220052),
    float2(-0.5486979, -0.09635002),
    float2(-0.5235587, 0.6594529),
    float2(-0.5170338, -0.6636339),
    float2(-0.5114055, 0.4373561),
    float2(-0.4844725, 0.2985838),
    float2(-0.4803245, 0.8482798),
    float2(-0.4651957, -0.5392771),
    float2(-0.4529685, 0.09942394),
    float2(-0.4523471, -0.3125569),
    float2(-0.4268422, 0.5644538),
    float2(-0.4187512, -0.8636028),
    float2(-0.4160798, -0.0844868),
    float2(-0.3751733, 0.2196607),
    float2(-0.3656596, -0.7324334),
    float2(-0.3286595, -0.2012637),
    float2(-0.3147397, -0.0006635741),
    float2(-0.3135846, 0.3636878),
    float2(-0.3042951, -0.4983553),
    float2(-0.2974239, 0.7496996),
    float2(-0.2903037, 0.8890813),
    float2(-0.2878664, -0.8622097),
    float2(-0.2588971, -0.653879),
    float2(-0.2555692, 0.5041648),
    float2(-0.2553292, -0.3389159),
    float2(-0.2401368, 0.2306108),
    float2(-0.2124457, -0.09935001),
    float2(-0.1877905, 0.1098409),
    float2(-0.1559879, 0.3356432),
    float2(-0.1499449, 0.7487829),
    float2(-0.146661, -0.9256138),
    float2(-0.1342774, 0.6185387),
    float2(-0.1224529, -0.3887629),
    float2(-0.116467, 0.8827716),
    float2(-0.1157598, -0.539999),
    float2(-0.09983152, -0.2407187),
    float2(-0.09953719, -0.78346),
    float2(-0.08604223, 0.4591112),
    float2(-0.02128129, 0.1551989),
    float2(-0.01478849, 0.6969455),
    float2(-0.01231739, -0.6752576),
    float2(-0.005001599, -0.004027164),
    float2(0.00248426, 0.567932),
    float2(0.00335562, 0.3472346),
    float2(0.009554717, -0.4025437),
    float2(0.02231783, -0.1349781),
    float2(0.04694207, -0.8347212),
    float2(0.05412609, 0.9042216),
    float2(0.05812819, -0.9826952),
    float2(0.1131321, -0.619306),
    float2(0.1170737, 0.6799788),
    float2(0.1275105, 0.05326218),
    float2(0.1393405, -0.2149568),
    float2(0.1457873, 0.1991508),
    float2(0.1474208, 0.5443151),
    float2(0.1497117, -0.3899909),
    float2(0.1923773, 0.3683496),
    float2(0.2110928, -0.7888536),
    float2(0.2148235, 0.9586087),
    float2(0.2152219, -0.1084362),
    float2(0.2189204, -0.9644538),
    float2(0.2220028, -0.5058427),
    float2(0.2251696, 0.779461),
    float2(0.2585723, 0.01621339),
    float2(0.2612841, -0.2832426),
    float2(0.2665483, -0.6422054),
    float2(0.2939872, 0.1673226),
    float2(0.3235748, 0.5643662),
    float2(0.3269232, 0.6984669),
    float2(0.3425438, -0.1783788),
    float2(0.3672505, 0.4398117),
    float2(0.3755714, -0.8814359),
    float2(0.379463, 0.2842356),
    float2(0.3822978, -0.381217),
    float2(0.4057849, -0.5227674),
    float2(0.4168737, -0.6936938),
    float2(0.4202749, 0.8369391),
    float2(0.4252189, 0.03818182),
    float2(0.4445904, -0.09360636),
    float2(0.4684285, 0.5885228),
    float2(0.4952184, -0.2319764),
    float2(0.5072351, 0.3683765),
    float2(0.5136194, -0.3944138),
    float2(0.519893, 0.7157083),
    float2(0.5277841, 0.1486474),
    float2(0.5474944, -0.7618791),
    float2(0.5692734, 0.4852227),
    float2(0.582229, -0.5125455),
    float2(0.583022, 0.008507785),
    float2(0.6500257, 0.3473313),
    float2(0.6621304, -0.6280518),
    float2(0.6674218, -0.2260806),
    float2(0.6741871, 0.6734863),
    float2(0.6753459, 0.1119422),
    float2(0.7083091, -0.4393666),
    float2(0.7106963, -0.102099),
    float2(0.7606754, 0.5743545),
    float2(0.7846709, 0.2282225),
    float2(0.7871446, 0.3891495),
    float2(0.8071781, -0.5257092),
    float2(0.8230689, 0.002674922),
    float2(0.8531976, -0.3256475),
    float2(0.8758298, -0.1824844),
    float2(0.8797691, 0.1284946),
    float2(0.926309, 0.3576975),
    float2(0.9608918, -0.03495717),
    float2(0.972032, 0.2271516)};
    */

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

float2 FindBlocker(float2 uv, float lightSizeUV, float zReceiver, float zVS,
                   depth2d<float> shadowMap, float near) {

  constexpr sampler s(coord::normalized, filter::nearest,
                      address::clamp_to_edge, compare_func::less,
                      lod_clamp(0.0f, 0.0f));
  float2 result;
  // This uses similar triangles to compute what
  // area of the shadow map we should search
  float searchWidth = lightSizeUV * saturate(zReceiver - near) / zReceiver;
  float blockerSum = 0;
  result.y = 0;
  for (int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; ++i) {

    float shadowMapDepth =
        shadowMap.sample(s, uv + poissonDisk[i] * searchWidth);
    // shadowMap.sample(s, uv + poissonDisk[i] * 0.015);
    // float shadowMapDepth = tDepthMap.SampleLevel(
    //    PointSampler, uv + poissonDisk[i] * searchWidth, 0);
    if (shadowMapDepth < (zReceiver - 0.00004f)) {
      blockerSum += shadowMapDepth;
      result.y += 1.0f;
    }
  }
  result.x = blockerSum / result.y;
  return result;
}

float z_clip_to_eye(float z, float u_LightNear) {
  float u_LightFar = 40.0f;
  return u_LightFar * u_LightNear /
         (u_LightFar - z * (u_LightFar - u_LightNear));
}
// Using similar triangles between the area light, the blocking plane and the
// surface point
float penumbra_radius_uv(float zReceiver, float zBlocker) {
  return abs(zReceiver - zBlocker) / zBlocker;
}

// ------------------------------------------------------------------

// Project UV size to the near plane of the light
float project_to_light_uv(float penumbra_radius, float z, float u_LightSize,
                          float u_LightNear) {
  return penumbra_radius * u_LightSize * u_LightNear / z;
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

  //convert to light projected space
  float4 wp = vertexIn.worldPos;
  wp.w = 1.0f;
  float4 shadowPos = light->VP * wp;
  shadowPos /= shadowPos.w;
  //we only remap xy coords to range [0,1] from [-1,1] since the z is already in that range
  float2 xy = shadowPos.xy;
  xy = xy * 0.5 + 0.5;
  xy.y = 1 - xy.y;

  //now we get also the position in light space, no projection
  float4 shadowVS = light->V * wp;
  float zVS = shadowVS.z /shadowVS.w;

  // find blockers
  //  float size = 0.005f;
  float size = light->lightSize;
  float2 q = FindBlocker(xy, size, shadowPos.z, zVS, shadowMap, light->near);
  // return half4(q.y/16.0f,q.y/16,q.y/16,1);
  // float distance = FindBlockerDistance(shadowPos.xyz, shadowMap, 2.0f);
  float distance = q.y > 1 ? q.x : -1;
  float lightFactor = 1.0f;
  if (distance > 0) {
    constexpr sampler s(coord::normalized, filter::linear,
                        address::clamp_to_edge, compare_func::less,
                        lod_clamp(0.0f, 0.0f));
    //    float shadowMapDepth = shadowMap.sample(s, xy);
    float shadowMapDepth = distance;
    float w = (shadowPos.z - shadowMapDepth) / shadowMapDepth;
    w = w * light->lightSize *
        (light->near / z_clip_to_eye(shadowPos.z, light->near));

    float avg_blocker_depth_vs = z_clip_to_eye(shadowMapDepth, light->near);
    float penumbra_radius =
        penumbra_radius_uv(shadowPos.z, avg_blocker_depth_vs);
    float filter_radius = project_to_light_uv(penumbra_radius, shadowPos.z,
                                              light->lightSize, light->near);
    w = filter_radius;
    /*
    float penumbraRatio = PenumbraSize(zReceiver, avgBlockerDepth);
    float filterRadiusUV =
        penumbraRatio * LIGHT_SIZE_UV * NEAR_PLANE / coords.z;
        */

    lightFactor =
        PCF_Filter(xy, shadowPos.z - 0.00005f, w, shadowMap, light->pcfsamples);
    /*
    const int neighborWidth = 3;
    const float neighbors =
        (neighborWidth * 2.0 + 1.0) * (neighborWidth * 2.0 + 1.0);
    float mapSize = 2048;
    float texelSize = 1.0 / mapSize;
    float total = 0.0;
    for (int x = -neighborWidth; x <= neighborWidth; x++) {
      for (int y = -neighborWidth; y <= neighborWidth; y++) {
        float shadow_sample = shadowMap.sample_compare(s, xy + float2(x, y) * w,
                                                       shadowPos.z - 0.00004f);
        total += shadow_sample;
      }
    }
    total /= neighbors;
    lightFactor = (total);
    */
  }

  // return half4(1,0,0,1);

  // Working pcf
  // float lightFactor =
  //    PCF_Filter(xy, shadowPos.z - 0.00005f, light->pcfsize, shadowMap,
  //    light->pcfsamples);

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
