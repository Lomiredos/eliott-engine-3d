#pragma once

#include "visu/render/Renderer.hpp"

#include <memory>

namespace ee::render {
void SetRenderer(std::unique_ptr<IRenderer> _renderer);
IRenderer &GetRenderer();
} // namespace ee::render
