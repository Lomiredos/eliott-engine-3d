#pragma once

// ---------------------------------------------------------------------------
// Image partagee entre le jeu (qui ecrit) et l'editeur (qui lit), sans copie
// par la socket : un bloc de memoire partagee nomme, avec deux buffers RGBA.
//
// Le jeu ecrit dans le buffer libre puis le publie (indice + compteur) ;
// l'editeur lit le dernier publie quand le compteur change. Taille maximale
// fixee a la creation (redimensionner = changer width/height, pas le bloc).
// POSIX shm_open/mmap ou Win32 CreateFileMapping selon la plateforme.
// ---------------------------------------------------------------------------

#include <cstdint>
#include <string>

namespace ee::ipc
{
    class SharedFrame
    {
    public:
        static constexpr int kMaxWidth = 3840;
        static constexpr int kMaxHeight = 2160;

        SharedFrame() = default;
        ~SharedFrame();
        SharedFrame(const SharedFrame &) = delete;
        SharedFrame &operator=(const SharedFrame &) = delete;

        // Editeur : cree le bloc (et le detruit a la destruction).
        bool create(const std::string &_name);
        // Jeu : ouvre le bloc cree par l'editeur.
        bool open(const std::string &_name);
        void close();
        bool valid() const { return m_base != nullptr; }

        // --- Cote jeu ---
        // Buffer ou ecrire la prochaine image (kMaxWidth*kMaxHeight*4 octets).
        uint8_t *backBuffer();
        // Publie le buffer ecrit, de taille _w x _h (lignes de bas en haut, comme OpenGL).
        void publish(int _w, int _h);

        // --- Cote editeur ---
        // Si une image plus recente que _lastCounter est dispo : renvoie true,
        // les pixels, la taille, et met a jour _lastCounter.
        bool latest(uint64_t &_lastCounter, const uint8_t *&_pixels, int &_w, int &_h) const;

    private:
        struct Header;
        Header *header() const;
        uint8_t *buffer(int _index) const;

        void *m_base = nullptr;
        std::size_t m_size = 0;
        std::string m_name;
        bool m_owner = false;
#ifdef _WIN32
        void *m_mapping = nullptr; // HANDLE
#endif
    };
}
