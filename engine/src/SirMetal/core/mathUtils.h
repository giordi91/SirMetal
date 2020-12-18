#include "simd/matrix_types.h"

/// Builds a translation matrix that translates by the supplied vector
matrix_float4x4 matrix_float4x4_translation(vector_float3 t);

/// Builds a scale matrix that uniformly scales all axes by the supplied factor
matrix_float4x4 matrix_float4x4_uniform_scale(float scale);
matrix_float4x4 matrix_float4x4_scale(simd_float3 scale);

/// Builds a rotation matrix that rotates about the supplied axis by an
/// angle (given in radians). The axis should be normalized.
matrix_float4x4 matrix_float4x4_rotation(vector_float3 axis, float angle);

/// Builds a symmetric perspective projection matrix with the supplied aspect ratio,
/// vertical field of view (in radians), and near and far distances
matrix_float4x4 matrix_float4x4_perspective(float aspect, float fovy, float near, float far);

matrix_float4x4 getMatrixFromComponents(simd_float3 t, simd_quatf r ,simd_float3 s);

matrix_float4x4 getIdentity();
