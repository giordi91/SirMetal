#pragma once

#import <string>
#import "../graphics/camera.h"

namespace SirMetal {
    namespace Editor {
        struct Settings {
            std::string m_projectName;
            CameraManipulationConfig m_cameraConfig;
        };
    }
}
