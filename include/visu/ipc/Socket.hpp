#pragma once

// ---------------------------------------------------------------------------
// Socket TCP locale (127.0.0.1) non bloquante, pour le lien editeur <-> jeu.
//
// Les messages sont des JSON prefixes par leur taille (4 octets, little
// endian). send() ne bloque jamais : ce qui ne part pas tout de suite est
// garde et renvoye au prochain send()/poll(). POSIX ou Winsock selon la
// plateforme ; rien de specifique ne fuit dans ce header.
// ---------------------------------------------------------------------------

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ee::ipc
{
    class Socket
    {
    public:
        Socket() = default;
        ~Socket();
        Socket(Socket &&_other) noexcept;
        Socket &operator=(Socket &&_other) noexcept;
        Socket(const Socket &) = delete;
        Socket &operator=(const Socket &) = delete;

        // Ecoute sur 127.0.0.1, port choisi par le systeme (renvoye dans _port).
        static Socket listen(uint16_t &_port);
        // Connexion a 127.0.0.1:_port (bloquante le temps de la connexion).
        static Socket connect(uint16_t _port);

        // Socket d'ecoute : renvoie la connexion entrante, ou une socket
        // invalide s'il n'y en a pas encore (non bloquant).
        Socket accept();

        bool valid() const { return m_handle != kInvalid; }
        // true si la connexion est fermee (par l'autre cote ou sur erreur).
        bool closed() const { return m_closed; }

        void send(const nlohmann::json &_msg);
        // Lit tout ce qui est arrive et ajoute les messages complets a _out.
        // Renvoie false si la connexion est fermee.
        bool poll(std::vector<nlohmann::json> &_out);

        void close();

    private:
        using Handle = std::uintptr_t; // int (POSIX) ou SOCKET (Windows)
        static constexpr Handle kInvalid = ~static_cast<Handle>(0);

        explicit Socket(Handle _h) : m_handle(_h) {}
        void flush();

        Handle m_handle = kInvalid;
        bool m_closed = false;
        std::string m_in;  // octets recus pas encore decoupes en messages
        std::string m_out; // octets pas encore envoyes
    };
}
