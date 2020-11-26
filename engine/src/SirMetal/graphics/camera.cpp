
#import <simd/simd.h>

#import "SirMetal/engine.h"
#import "SirMetal/core/input.h"
#include "SirMetal/MBEMathUtilities.h"
#include "SirMetal/graphics/camera.h"
#include "SDL.h"

namespace SirMetal {

    void EditorFPSCameraController::update(const CameraManipulationConfig& camConfig, Input *input) {

        //resetting position
        simd_float4 pos = m_camera->viewMatrix.columns[3];
        m_camera->viewMatrix.columns[3] = simd_float4{0, 0, 0, 1};

        //applying rotation if mouse is pressed
        if (input->mouse.buttons[SDL_BUTTON_LEFT]) {
            simd_float4 up = simd_float4{0, 1, 0, 0};
            const float rotationYFactor = input->mouse.position.xRel * camConfig.leftRightLookDirection * camConfig.lookSpeed;
            const matrix_float4x4 camRotY = matrix_float4x4_rotation(vector_float3{up.x, up.y, up.z}, rotationYFactor);
            m_camera->viewMatrix = matrix_multiply(camRotY, m_camera->viewMatrix);

            simd_float4 side = m_camera->viewMatrix.columns[0];
            const float rotationXFactor = input->mouse.position.yRel * camConfig.upDownLookDirection * camConfig.lookSpeed;
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
        float applicationFactor = input->isKeyDown(SDL_SCANCODE_A);
        float leftRightFactor = camConfig.leftRightMovementDirection * camConfig.movementSpeed;
        pos += side * (-leftRightFactor) * applicationFactor;

        applicationFactor = input->isKeyDown(SDL_SCANCODE_D);
        pos += side * (leftRightFactor) * applicationFactor;

        //forward and back
        float fbFactor = camConfig.forwardBackMovementDirection * camConfig.movementSpeed;
        applicationFactor = input->isKeyDown(SDL_SCANCODE_W);
        pos += forward * (-fbFactor) * applicationFactor;

        applicationFactor = input->isKeyDown(SDL_SCANCODE_S);
        pos += forward * fbFactor * applicationFactor;

        //up and down
        float udFactor = camConfig.upDownMovementDirection * camConfig.movementSpeed;
        applicationFactor = input->isKeyDown(SDL_SCANCODE_Q);
        pos += vector_float4{0, 1, 0, 0} * (applicationFactor) * (-udFactor);

        applicationFactor = input->isKeyDown(SDL_SCANCODE_E);
        pos += vector_float4{0, 1, 0, 0} * applicationFactor * (udFactor);

        m_camera->viewMatrix.columns[3] = pos;
        m_camera->viewInverse = simd_inverse(m_camera->viewMatrix);


    }

    void EditorFPSCameraController::setPosition(float x, float y, float z) {
        //Should this update the inverse matrix? Up for discussion
        //for now the only place where that happens is in update
        m_camera->viewMatrix.columns[3] = simd_float4{x, y, z, 1.0f};
    }

    void EditorFPSCameraController::updateProjection(float screenWidth, float screenHeight)
    {
        m_camera->screenWidth = screenWidth;
        m_camera->screenHeight = screenHeight;
        
        const float aspect = screenWidth / screenHeight;
        const float fov = m_camera->fov;
        const float near = m_camera->nearPlane;
        const float far = m_camera->farPlane;
        m_camera->projection = matrix_float4x4_perspective(aspect, fov, near, far);
        m_camera->VP = simd_mul(m_camera->projection, m_camera->viewInverse);
    }
}
