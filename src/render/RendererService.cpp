#include "visu/render/RendererService.hpp"
#include "visu/render/GLRenderer.hpp"

#include <cstdio>

namespace ee::render
{
    namespace
    {
        std::unique_ptr<IRenderer> g_renderer;
        bool g_getCalled = false; // true apres le tout premier GetRenderer()
    }

    void SetRenderer(std::unique_ptr<IRenderer> _renderer)
    {
        if (g_getCalled)
        {
            std::fprintf(stderr,
                         "[ee::render] SetRenderer() ignore : appele apres le "
                         "premier GetRenderer() (un GLRenderer par defaut est "
                         "deja en cours d'utilisation).\n");
            return;
        }
        g_renderer = std::move(_renderer);
    }

    IRenderer &GetRenderer()
    {
        g_getCalled = true;
        if (!g_renderer)
            g_renderer = std::make_unique<GLRenderer>();
        return *g_renderer;
    }
}
