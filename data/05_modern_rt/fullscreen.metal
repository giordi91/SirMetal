#include <metal_stdlib>

using namespace metal;

struct OutVertex {
  float4 position [[position]];
  float2 uv;
  float2 padding;
};

vertex OutVertex vertex_project( uint vid [[vertex_id]]) {


  OutVertex vertexOut;
  float x = -1.0 + float((vid & 1) << 2);
  float y = -1.0 + float((vid & 2) << 1);
  float uvx = (x+1.0)*0.5;
  float uvy = (y+1.0)*0.5;
  vertexOut.position.x = x;
  vertexOut.position.y = y;
  vertexOut.position.z = 0.0f;
  vertexOut.position.w = 1.0f;
  vertexOut.uv.x = uvx;
  vertexOut.uv.y = uvy;

  return vertexOut;
}

fragment half4 fragment_flatcolor(OutVertex vertexIn [[stage_in]],
                                  texture2d<float> blitTex [[texture(0)]]) {

  float2 uv = vertexIn.uv;
  constexpr sampler linearSampler(coord::normalized, filter::nearest);
  float4 c = blitTex.sample(linearSampler, uv);
  //quick and dirty gamma
  c = sqrt(c);
  return half4(c.x,c.y,c.z,1.0h);
}
