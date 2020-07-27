#pragma

#import <simd/matrix_types.h>

namespace SirMetal {
    struct Camera {
        matrix_float4x4 viewMatrix;
        matrix_float4x4 projection;
        matrix_float4x4 VP;
        float screenWidth;
        float screenHeight;
        float nearPlane;
        float farPlane;
    };


    class CameraController
    {
    public:
        void setCamera(Camera* camera){m_camera = camera;}
        virtual void move() =0;
        virtual void rotate() =0;
    protected:
        Camera* m_camera = nullptr;
    };


    class FPSCameraController: public CameraController
    {
        void move() override;
        void rotate() override;

    };


}


