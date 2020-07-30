
#include "cameraSettingsWidget.h"
#include "../../vendors/imgui/imgui.h"
#import "camera.h"

namespace SirMetal {
    namespace Editor {
        void SirMetal::Editor::CameraSettingsWidget::render(CameraManipulationConfig *config, bool* showWindow) {

            ImGui::Begin("Camera settings", showWindow);
            bool lrlookDir = config->leftRightLookDirection < 0.0f;
            if(ImGui::Checkbox("invert L/R look direction", &lrlookDir))
            {
                config->leftRightLookDirection = lrlookDir ?- 1.0f : 1.0f;
            }
            bool udlookDir = config->upDownLookDirection< 0.0f;
            if(ImGui::Checkbox("invert U/D look direction", &udlookDir))
            {
                config->upDownLookDirection= udlookDir? -1.0f : 1.0f;
            }
            bool lrmoveDir = config->leftRightMovementDirection< 0.0f;
            if(ImGui::Checkbox("invert L/R move direction", &lrmoveDir))
            {
                config->leftRightMovementDirection= lrmoveDir? -1.0f : 1.0f;
            }
            bool fbmoveDir = config->forwardBackMovementDirection< 0.0f;
            if(ImGui::Checkbox("invert F/B move direction", &fbmoveDir))
            {
                config->leftRightMovementDirection= fbmoveDir? -1.0f : 1.0f;
            }
            bool udmoveDir = config->upDownMovementDirection< 0.0f;
            if(ImGui::Checkbox("invert U/D move direction", &udmoveDir))
            {
                config->upDownMovementDirection= udmoveDir? -1.0f : 1.0f;
            }
            ImGui::SliderFloat("movement speed", &config->movementSpeed, 0.0001f, 0.5f);
            ImGui::SliderFloat("look speed", &config->lookSpeed, 0.0001f, 0.1f);

            ImGui::End();

        }
    }
}
