#include <metal_stdlib>

using namespace metal;

struct OutVertex {
  float4 position [[position]];
};

vertex OutVertex vertex_project( uint vid [[vertex_id]]) {


OutVertex vertexOut;
float x = -1.0 + float((vid & 1) << 2);
float y = -1.0 + float((vid & 2) << 1);
vertexOut.position.x = x;
vertexOut.position.y = y;
vertexOut.position.z = 0.0f;
vertexOut.position.w = 1.0f;
return vertexOut;
}


struct FragmentOut {
  uint position [[color(0)]];
  float2 uv [[color(1)]];
};
fragment FragmentOut fragment_flatcolor(OutVertex vertexIn [[stage_in]]) {
  FragmentOut fout;
  fout.position = 0xFFFFFFFF;
  fout.uv = 0;
  return fout;
}
