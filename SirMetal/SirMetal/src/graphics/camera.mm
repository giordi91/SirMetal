
#import <simd/simd.h>
#include "camera.h"

#import "engineContext.h"
#import "input.h"
#import "MBEMathUtilities.h"

namespace SirMetal
{

    void FPSCameraController::update(Input *input, float screenWidth, float screenHeight) {

        if ((SirMetal::CONTEXT->flags.interaction & SirMetal::InteractionFlagsBits::InteractionViewportFocused) > 0) {

            simd_float4 pos = m_camera->viewMatrix.columns[3];
            //resetting position
            m_camera->viewMatrix.columns[3] = simd_float4{0, 0, 0, 1};

            simd_float4 up = simd_float4{0, 1, 0, 0};
            const matrix_float4x4 camRotY = matrix_float4x4_rotation(vector_float3{up.x, up.y, up.z}, input->m_mouseDeltaX * -0.01f);
            if(input->m_mouse[SirMetal::MOUSE_BUTTONS::LEFT]){
                m_camera->viewMatrix = matrix_multiply(camRotY, m_camera->viewMatrix);
            }


            simd_float4 side = m_camera->viewMatrix.columns[0];
            const matrix_float4x4 camRotX = matrix_float4x4_rotation(vector_float3{side.x, side.y, side.z}, input->m_mouseDeltaY * 0.01f);
            if(input->m_mouse[SirMetal::MOUSE_BUTTONS::LEFT]) {
                m_camera->viewMatrix = matrix_multiply(camRotX, m_camera->viewMatrix);
            }

            simd_float4 forward= m_camera->viewMatrix.columns[2];

            if (input->isKeyDown(SirMetal::KEY_CODES::A)) {
                pos -= side*0.1;
            }
            if (input->isKeyDown(SirMetal::KEY_CODES::D)) {
                pos += side*0.1;
            }
            if (input->isKeyDown(SirMetal::KEY_CODES::W)) {
                pos -= forward*0.1;

            }
            if (input->isKeyDown(SirMetal::KEY_CODES::S)) {
                pos += forward*0.1;
            }
            if (input->isKeyDown(SirMetal::KEY_CODES::Q)) {
                pos += vector_float4{0, 0.1, 0, 0};
            }
            if (input->isKeyDown(SirMetal::KEY_CODES::E)) {
                pos += vector_float4{0, -0.1, 0, 0};
            }


            m_camera->viewMatrix.columns[3] = pos;
        }


        m_camera->viewInverse = simd_inverse(m_camera->viewMatrix);

        m_camera->screenWidth = screenWidth;
        m_camera->screenHeight= screenHeight;

        const float aspect = screenWidth / screenHeight;
        const float fov = m_camera->fov;
        const float near = m_camera->nearPlane;
        const float far = m_camera->farPlane;
        m_camera->projection = matrix_float4x4_perspective(aspect, fov, near, far);
        m_camera->VP = simd_mul(m_camera->projection,m_camera->viewInverse);
    }

    void FPSCameraController::setPosition(float x, float y, float z) {
        //Should this update the inverse matrix? Up for discussion
        //for now the only place where that happens is in update
        m_camera->viewMatrix.columns[3] = simd_float4{x,y,z,1.0f};

    }
}