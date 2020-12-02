#pragma once

#import <simd/matrix_types.h>

namespace SirMetal {
// forward declares
class Input;

struct CameraManipulationConfig {
  float leftRightLookDirection;
  float upDownLookDirection;
  float leftRightMovementDirection;
  float forwardBackMovementDirection;
  float upDownMovementDirection;
  float movementSpeed;
  float lookSpeed;
};

struct Camera {
  matrix_float4x4 viewMatrix;
  matrix_float4x4 viewInverse;
  matrix_float4x4 projection;
  matrix_float4x4 VP;
  matrix_float4x4 VPInverse;
  float screenWidth;
  float screenHeight;
  float nearPlane;
  float farPlane;
  float fov;
};

class CameraController {
public:
  void setCamera(Camera *camera) { m_camera = camera; }

  virtual void update(const CameraManipulationConfig &camConfig,
                      Input *input) = 0;
  virtual void updateProjection(float screenWidth, float screenHeight) = 0;

  virtual void setPosition(float x, float y, float z) = 0;

protected:
  Camera *m_camera = nullptr;
};

class FPSCameraController : public CameraController {
public:
  void update(const CameraManipulationConfig &camConfig, Input *input) override;
  void updateProjection(float screenWidth, float screenHeight) override;
  void setPosition(float x, float y, float z) override;
};

} // namespace SirMetal
