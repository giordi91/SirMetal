#pragma once

#include "stdint.h"
namespace SirMetal {

// Common graphics defines

enum class SHADER_TYPE { VERTEX = 0, FRAGMENT, COMPUTE, INVALID };

// angles
static constexpr float PI = 3.14159265358979323846f;
static constexpr double PI_D = 3.141592653589793238462643383279502884;
static constexpr float TO_RAD = static_cast<float>(PI_D / 180.0);
static constexpr double TO_RAD_D = PI_D / 180.0;
static constexpr float TO_DEG = static_cast<float>(180.0 / PI_D);
static constexpr double TO_DEG_D = 180.0 / PI_D;

// rendering
enum class RESOURCE_STATE {
  GENERIC,
  RENDER_TARGET,
  DEPTH_RENDER_TARGET,
  SHADER_READ_RESOURCE,
  RANDOM_WRITE
};
static constexpr double NS_TO_SECONDS = 1e-9;

static constexpr uint64_t MB_TO_BYTE = 1024 * 1024;
static constexpr double BYTE_TO_MB_D = 1.0 / MB_TO_BYTE;
static constexpr float BYTE_TO_MB = BYTE_TO_MB_D;

}  // namespace BlackHole