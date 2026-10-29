#pragma once

// Camera du jeu quand la scene n'a pas d'entite CameraComponent. A part de
// GameSession : l'editeur s'en sert (picking en pause) sans tirer l'ECS.

#include "visu/render/Renderer.hpp"

namespace ee::runtime
{
    // Camera quand la scene n'a pas d'entite CameraComponent.
    inline ee::render::Camera defaultCamera()
    {
        ee::render::Camera cam;
        cam.pos = {0.0f, 3.0f, 12.0f};
        cam.yaw = -1.5707963f;
        cam.pitch = -0.15f;
        return cam;
    }
}
