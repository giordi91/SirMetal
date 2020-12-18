#include "mathUtils.h"
#include <math.h>
#include <simd/simd.h>

matrix_float4x4 matrix_float4x4_translation(vector_float3 t) {
  vector_float4 X = {1.0f, 0.0f, 0.0f, 0.0f};
  vector_float4 Y = {0.0f, 1.0f, 0.0f, 0.0f};
  vector_float4 Z = {0.0f, 0.0f, 1.0f, 0.0f};
  vector_float4 W = {t.x, t.y, t.z, 1.0f};

  matrix_float4x4 mat = {X, Y, Z, W};
  return mat;
}

matrix_float4x4 getIdentity() {
  vector_float4 X = {1.0f, 0.0f, 0.0f, 0.0f};
  vector_float4 Y = {0.0f, 1.0f, 0.0f, 0.0f};
  vector_float4 Z = {0.0f, 0.0f, 1.0f, 0.0f};
  vector_float4 W = {0.0f, 0.0f, 0.0f, 1.0f};
  matrix_float4x4 mat = {X, Y, Z, W};
  return mat;
}

matrix_float4x4 matrix_float4x4_uniform_scale(float scale) {
  vector_float4 X = {scale, 0.0f, 0.0f, 0.0f};
  vector_float4 Y = {0.0f, scale, 0.0f, 0.0f};
  vector_float4 Z = {0.0f, 0.0f, scale, 0.0f};
  vector_float4 W = {0.0f, 0.0f, 0.0f, 1.0f};

  matrix_float4x4 mat = {X, Y, Z, W};
  return mat;
}
matrix_float4x4 matrix_float4x4_scale(simd_float3 scale) {
  vector_float4 X = {scale.x, 0.0f, 0.0f, 0.0f};
  vector_float4 Y = {0.0f, scale.y, 0.0f, 0.0f};
  vector_float4 Z = {0.0f, 0.0f, scale.z, 0.0f};
  vector_float4 W = {0.0f, 0.0f, 0.0f, 1.0f};

  matrix_float4x4 mat = {X, Y, Z, W};
  return mat;
}

matrix_float4x4 matrix_float4x4_rotation(vector_float3 axis, float angle) {
  float c = cos(angle);
  float s = sin(angle);

  vector_float4 X;
  X.x = axis.x * axis.x + (1.0f - axis.x * axis.x) * c;
  X.y = axis.x * axis.y * (1.0f - c) - axis.z * s;
  X.z = axis.x * axis.z * (1.0f - c) + axis.y * s;
  X.w = 0.0f;

  vector_float4 Y;
  Y.x = axis.x * axis.y * (1.0f - c) + axis.z * s;
  Y.y = axis.y * axis.y + (1.0f - axis.y * axis.y) * c;
  Y.z = axis.y * axis.z * (1.0f - c) - axis.x * s;
  Y.w = 0.0f;

  vector_float4 Z;
  Z.x = axis.x * axis.z * (1.0f - c) - axis.y * s;
  Z.y = axis.y * axis.z * (1.0f - c) + axis.x * s;
  Z.z = axis.z * axis.z + (1.0f - axis.z * axis.z) * c;
  Z.w = 0.0f;

  vector_float4 W;
  W.x = 0.0f;
  W.y = 0.0f;
  W.z = 0.0f;
  W.w = 1.0f;

  matrix_float4x4 mat = {X, Y, Z, W};
  return mat;
}

matrix_float4x4 matrix_float4x4_perspective(float aspect, float fovy,
                                            float near, float far) {
  float yScale = 1.0f / tan(fovy * 0.5f);
  float xScale = yScale / aspect;
  float zRange = far - near;
  float zScale = -(far + near) / zRange;
  float wzScale = -2.0f * far * near / zRange;

  vector_float4 P = {xScale, 0.0f, 0.0f, 0.0f};
  vector_float4 Q = {0.0f, yScale, 0.0f, 0.0f};
  vector_float4 R = {0.0f, 0.0f, zScale, -1.0f};
  vector_float4 S = {0.0f, 0.0f, wzScale, 0.0f};

  matrix_float4x4 mat = {P, Q, R, S};
  return mat;
}
matrix_float4x4 getMatrixFromComponents(simd_float3 t, simd_quatf r,
                                        simd_float3 s) {
  auto tm = matrix_float4x4_translation(t);
  auto sm = matrix_float4x4_scale(s);
  auto rm = simd_matrix4x4(r);
  return simd_mul(tm ,simd_mul(rm , sm));
}
