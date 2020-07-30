#pragma once

namespace SirMetal {
    class CameraManipulationConfig;
    namespace Editor {

        class CameraSettingsWidget {
        public:
            void render(CameraManipulationConfig *config, bool *showWindow);

        };
    }
}


