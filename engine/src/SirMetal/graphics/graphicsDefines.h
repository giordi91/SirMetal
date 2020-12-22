#pragma once

#include "stdint.h"
namespace SirMetal {

// Common graphics defines

enum class SHADER_TYPE { RASTER = 0, COMPUTE, INVALID };

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

}  // namespace BlackHole