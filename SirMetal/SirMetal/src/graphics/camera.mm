
#import <simd/simd.h>

#import "engineContext.h"
#import "MBEMathUtilities.h"

namespace SirMetal {

    void FPSCameraController::update(Input *input, float screenWidth, float screenHeight) {


        const CameraManipulationConfig &camConfig = CONTEXT->settings.m_cameraConfig;

        //resetting position
        simd_float4 pos = m_camera->viewMatrix.columns[3];
        m_camera->viewMatrix.columns[3] = simd_float4{0, 0, 0, 1};

        //applying rotation if mouse is pressed
        if (input->m_mouse[SirMetal::MOUSE_BUTTONS::LEFT]) {
            simd_float4 up = simd_float4{0, 1, 0, 0};
            const float rotationYFactor = input->m_mouseDeltaX * camConfig.leftRightLookDirection * camConfig.lookSpeed;
            const matrix_float4x4 camRotY = matrix_float4x4_rotation(vector_float3{up.x, up.y, up.z}, rotationYFactor);
            m_camera->viewMatrix = matrix_multiply(camRotY, m_camera->viewMatrix);

            simd_float4 side = m_camera->viewMatrix.columns[0];
            const float rotationXFactor = input->m_mouseDeltaY * camConfig.upDownLookDirection * camConfig.lookSpeed;
            const matrix_float4x4 camRotX = matrix_float4x4_rotation(vector_float3{side.x, side.y, side.z}, rotationXFactor);
            m_camera->viewMatrix = matrix_multiply(camRotX, m_camera->viewMatrix);
        }

        simd_float4 forward = m_camera->viewMatrix.columns[2];
        simd_float4 side = m_camera->viewMatrix.columns[0];

        //movement factors

        //for each button we apply a factor and apply a transformation
        //if the button is not pressed factor will be zero.
        //This avoids us to do lots of if(). trading some ALU
        //work for branches.
        //moving left and right
        float applicationFactor = input->isKeyDown(SirMetal::KEY_CODES::A);
        float leftRightFactor = camConfig.leftRightMovementDirection * camConfig.movementSpeed;
        pos += side * (-leftRightFactor) * applicationFactor;

        applicationFactor = input->isKeyDown(SirMetal::KEY_CODES::D);
        pos += side * leftRightFactor * applicationFactor;

        //forward and back
        float fbFactor = camConfig.forwardBackMovementDirection * camConfig.movementSpeed;
        applicationFactor = input->isKeyDown(SirMetal::KEY_CODES::W);
        pos += forward * fbFactor * applicationFactor;

        applicationFactor = input->isKeyDown(SirMetal::KEY_CODES::S);
        pos += forward * (-fbFactor) * applicationFactor;

        //up and down
        float udFactor = camConfig.upDownMovementDirection * camConfig.movementSpeed;
        applicationFactor = input->isKeyDown(SirMetal::KEY_CODES::Q);
        pos += vector_float4{0, 1, 0, 0} * applicationFactor * udFactor;

        applicationFactor = input->isKeyDown(SirMetal::KEY_CODES::E);
        pos += vector_float4{0, 1, 0, 0} * applicationFactor * (-udFactor);

        m_camera->viewMatrix.columns[3] = pos;


        m_camera->viewInverse = simd_inverse(m_camera->viewMatrix);

        m_camera->screenWidth = screenWidth;
        m_camera->screenHeight = screenHeight;

        const float aspect = screenWidth / screenHeight;
        const float fov = m_camera->fov;
        const float near = m_camera->nearPlane;
        const float far = m_camera->farPlane;
        m_camera->projection = matrix_float4x4_perspective(aspect, fov, near, far);
        m_camera->VP = simd_mul(m_camera->projection, m_camera->viewInverse);
    }

    void FPSCameraController::setPosition(float x, float y, float z) {
        //Should this update the inverse matrix? Up for discussion
        //for now the only place where that happens is in update
        m_camera->viewMatrix.columns[3] = simd_float4{x, y, z, 1.0f};

    }
}