#pragma once

#include <string>

// ---------------------------------------------------------------------------
// Fenetre du jeu, vue depuis les systemes (sans GLFW) :
//
//   ee::core::Window &win = ee::core::Window::getInstance();
//   float aspect = win.aspect();
//   win.setCursorLocked(true);
//
// La taille est celle de l'image rendue cette frame : la fenetre en jeu seul,
// le panneau preview quand le jeu tourne dans l'editeur. Elle est mise a jour
// par le runtime AVANT chaque tick.
//
// En preview (isEmbedded()), il n'y a pas de vraie fenetre : setTitle,
// setCursorLocked et close sont memorises mais sans effet.
// ---------------------------------------------------------------------------
namespace ee::core
{
    class Window
    {
    private:
        int m_width = 0;
        int m_height = 0;
        bool m_embedded = false;

        std::string m_title;
        bool m_titleDirty = false;

        bool m_cursorLocked = false;
        bool m_cursorDirty = false;

        bool m_closeRequested = false;

        Window() {}

    public:
        static Window &getInstance()
        {
            static Window instance = Window();
            return instance;
        }

        // --- Pour les systemes ---------------------------------------------

        int width() const { return m_width; }
        int height() const { return m_height; }
        float aspect() const
        {
            return m_height > 0 ? static_cast<float>(m_width) / static_cast<float>(m_height) : 1.0f;
        }

        // Vrai quand le jeu tourne dans la preview de l'editeur.
        bool isEmbedded() const { return m_embedded; }

        const std::string &title() const { return m_title; }
        void setTitle(const std::string &_title)
        {
            m_title = _title;
            m_titleDirty = true;
        }

        // Curseur cache et bloque dans la fenetre (camera FPS...).
        bool isCursorLocked() const { return m_cursorLocked; }
        void setCursorLocked(bool _locked)
        {
            if (_locked == m_cursorLocked)
                return;
            m_cursorLocked = _locked;
            m_cursorDirty = true;
        }

        // Demande la fermeture du jeu a la fin de la frame.
        void close() { m_closeRequested = true; }

        // --- Reserve au runtime (App.hpp) ----------------------------------

        void runtimeInit(const std::string &_title, bool _embedded)
        {
            m_width = m_height = 0;
            m_embedded = _embedded;
            m_title = _title;
            m_titleDirty = false;
            m_cursorLocked = false;
            m_cursorDirty = false;
            m_closeRequested = false;
        }
        void runtimeSetSize(int _w, int _h)
        {
            m_width = _w;
            m_height = _h;
        }
        // Renvoient vrai une seule fois apres un changement a appliquer.
        bool runtimeTakeTitleChange()
        {
            bool d = m_titleDirty;
            m_titleDirty = false;
            return d;
        }
        bool runtimeTakeCursorChange()
        {
            bool d = m_cursorDirty;
            m_cursorDirty = false;
            return d;
        }
        bool runtimeCloseRequested() const { return m_closeRequested; }
    };
} // namespace ee::core
