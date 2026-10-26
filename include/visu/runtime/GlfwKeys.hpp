#pragma once

// Table GLFW <-> ee::input::Key, partagee par le jeu (sa fenetre) et
// l'editeur (Play dans la preview) : les deux lisent les touches par
// POSITION physique (GLFW_KEY_* = disposition US), jamais selon la langue.

#include "input/Keys.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace ee::runtime
{
    namespace detail
    {
        // Table GLFW <-> ee::input::Key, dans le MEME ORDRE que l'enum Key
        // (meme convention que s_keyTable cote SDL dans eliott-input).
        // La taille est deduite du tableau -> jamais de constante a resynchroniser
        // a la main si Keys.hpp change.
        inline const int *glfwKeyTable(int &_count)
        {
            static constexpr int table[] = {
                // Letters
                GLFW_KEY_A, GLFW_KEY_B, GLFW_KEY_C, GLFW_KEY_D, GLFW_KEY_E, GLFW_KEY_F,
                GLFW_KEY_G, GLFW_KEY_H, GLFW_KEY_I, GLFW_KEY_J, GLFW_KEY_K, GLFW_KEY_L,
                GLFW_KEY_M, GLFW_KEY_N, GLFW_KEY_O, GLFW_KEY_P, GLFW_KEY_Q, GLFW_KEY_R,
                GLFW_KEY_S, GLFW_KEY_T, GLFW_KEY_U, GLFW_KEY_V, GLFW_KEY_W, GLFW_KEY_X,
                GLFW_KEY_Y, GLFW_KEY_Z,
                // Numbers
                GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4,
                GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9,
                // Function keys
                GLFW_KEY_F1, GLFW_KEY_F2, GLFW_KEY_F3, GLFW_KEY_F4, GLFW_KEY_F5, GLFW_KEY_F6,
                GLFW_KEY_F7, GLFW_KEY_F8, GLFW_KEY_F9, GLFW_KEY_F10, GLFW_KEY_F11, GLFW_KEY_F12,
                // Navigation
                GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT,
                GLFW_KEY_HOME, GLFW_KEY_END, GLFW_KEY_PAGE_UP, GLFW_KEY_PAGE_DOWN,
                GLFW_KEY_INSERT, GLFW_KEY_DELETE,
                // Modifiers
                GLFW_KEY_LEFT_SHIFT, GLFW_KEY_RIGHT_SHIFT,
                GLFW_KEY_LEFT_CONTROL, GLFW_KEY_RIGHT_CONTROL,
                GLFW_KEY_LEFT_ALT, GLFW_KEY_RIGHT_ALT,
                GLFW_KEY_LEFT_SUPER, GLFW_KEY_RIGHT_SUPER,
                // Special
                GLFW_KEY_SPACE, GLFW_KEY_ENTER, GLFW_KEY_BACKSPACE,
                GLFW_KEY_TAB, GLFW_KEY_ESCAPE, GLFW_KEY_CAPS_LOCK,
                // Punctuation
                GLFW_KEY_MINUS, GLFW_KEY_EQUAL,
                GLFW_KEY_LEFT_BRACKET, GLFW_KEY_RIGHT_BRACKET,
                GLFW_KEY_BACKSLASH, GLFW_KEY_SEMICOLON,
                GLFW_KEY_APOSTROPHE, GLFW_KEY_GRAVE_ACCENT,
                GLFW_KEY_COMMA, GLFW_KEY_PERIOD, GLFW_KEY_SLASH,
                // System
                GLFW_KEY_PRINT_SCREEN, GLFW_KEY_SCROLL_LOCK, GLFW_KEY_PAUSE,
                // Numpad
                GLFW_KEY_KP_0, GLFW_KEY_KP_1, GLFW_KEY_KP_2, GLFW_KEY_KP_3, GLFW_KEY_KP_4,
                GLFW_KEY_KP_5, GLFW_KEY_KP_6, GLFW_KEY_KP_7, GLFW_KEY_KP_8, GLFW_KEY_KP_9,
                GLFW_KEY_KP_ADD, GLFW_KEY_KP_SUBTRACT, GLFW_KEY_KP_MULTIPLY, GLFW_KEY_KP_DIVIDE,
                GLFW_KEY_KP_ENTER, GLFW_KEY_KP_DECIMAL, GLFW_KEY_NUM_LOCK,
            };
            // Si Keys.hpp gagne/perd une entree sans que cette table suive, ca
            // casse ici a la compilation plutot qu'en silence a l'execution.
            static_assert(sizeof(table) / sizeof(table[0]) ==
                              static_cast<int>(ee::input::Key::NumLock) + 1,
                          "glfwKeyTable() desynchronise de l'enum ee::input::Key");
            _count = static_cast<int>(sizeof(table) / sizeof(table[0]));
            return table;
        }

    }
}
