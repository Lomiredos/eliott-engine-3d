#include "visu/ipc/Socket.hpp"

#include <cstdio>
#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace
{
#ifdef _WIN32
    using Native = SOCKET;
    const Native kNativeInvalid = INVALID_SOCKET;

    // WSAStartup une fois pour tout le process.
    void ensureWinsock()
    {
        static bool s_done = []()
        {
            WSADATA data;
            return WSAStartup(MAKEWORD(2, 2), &data) == 0;
        }();
        (void)s_done;
    }
    bool wouldBlock() { return WSAGetLastError() == WSAEWOULDBLOCK; }
    // Pas d'heritage par les process enfants (le jeu lance par l'editeur).
    void noInherit(Native _s) { SetHandleInformation(reinterpret_cast<HANDLE>(_s), HANDLE_FLAG_INHERIT, 0); }
    void closeNative(Native _s) { closesocket(_s); }
    void setNonBlocking(Native _s)
    {
        u_long on = 1;
        ioctlsocket(_s, FIONBIO, &on);
    }
    const int kSendFlags = 0;
#else
    using Native = int;
    const Native kNativeInvalid = -1;

    void ensureWinsock() {}
    bool wouldBlock() { return errno == EAGAIN || errno == EWOULDBLOCK; }
    // Pas d'heritage par les process enfants (le jeu lance par l'editeur).
    void noInherit(Native _s) { fcntl(_s, F_SETFD, fcntl(_s, F_GETFD) | FD_CLOEXEC); }
    void closeNative(Native _s) { ::close(_s); }
    void setNonBlocking(Native _s) { fcntl(_s, F_SETFL, fcntl(_s, F_GETFL, 0) | O_NONBLOCK); }
    const int kSendFlags = MSG_NOSIGNAL; // pas de SIGPIPE si l'autre cote est parti
#endif

    // Messages tres gros = flux corrompu : on coupe plutot que d'allouer.
    constexpr uint32_t kMaxMessage = 64u * 1024u * 1024u;

    void noDelay(Native _s)
    {
        int on = 1;
        setsockopt(_s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char *>(&on), sizeof(on));
    }

    sockaddr_in loopback(uint16_t _port)
    {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(_port);
        return addr;
    }
}

namespace ee::ipc
{
    Socket::~Socket() { close(); }

    Socket::Socket(Socket &&_other) noexcept { *this = std::move(_other); }

    Socket &Socket::operator=(Socket &&_other) noexcept
    {
        if (this != &_other)
        {
            close();
            m_handle = _other.m_handle;
            m_closed = _other.m_closed;
            m_in = std::move(_other.m_in);
            m_out = std::move(_other.m_out);
            _other.m_handle = kInvalid;
        }
        return *this;
    }

    Socket Socket::listen(uint16_t &_port)
    {
        ensureWinsock();
        Native s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == kNativeInvalid)
            return {};
        noInherit(s);

        sockaddr_in addr = loopback(0); // port 0 : le systeme en choisit un libre
        if (::bind(s, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0 ||
            ::listen(s, 1) != 0)
        {
            closeNative(s);
            return {};
        }
        socklen_t len = sizeof(addr);
        getsockname(s, reinterpret_cast<sockaddr *>(&addr), &len);
        _port = ntohs(addr.sin_port);

        setNonBlocking(s);
        return Socket(static_cast<Handle>(s));
    }

    Socket Socket::connect(uint16_t _port)
    {
        ensureWinsock();
        Native s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == kNativeInvalid)
            return {};
        noInherit(s);

        sockaddr_in addr = loopback(_port);
        if (::connect(s, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0)
        {
            closeNative(s);
            return {};
        }
        noDelay(s);
        setNonBlocking(s);
        return Socket(static_cast<Handle>(s));
    }

    Socket Socket::accept()
    {
        if (!valid())
            return {};
        Native c = ::accept(static_cast<Native>(m_handle), nullptr, nullptr);
        if (c == kNativeInvalid)
            return {}; // rien en attente (ou erreur : on reessaiera)
        noInherit(c);
        noDelay(c);
        setNonBlocking(c);
        return Socket(static_cast<Handle>(c));
    }

    void Socket::send(const nlohmann::json &_msg)
    {
        if (!valid() || m_closed)
            return;
        std::string body = _msg.dump();
        uint32_t n = static_cast<uint32_t>(body.size());
        const char header[4] = {static_cast<char>(n & 0xFF), static_cast<char>((n >> 8) & 0xFF),
                                static_cast<char>((n >> 16) & 0xFF), static_cast<char>((n >> 24) & 0xFF)};
        m_out.append(header, 4);
        m_out += body;
        flush();
    }

    void Socket::flush()
    {
        while (!m_out.empty() && !m_closed)
        {
            int sent = ::send(static_cast<Native>(m_handle), m_out.data(),
                              static_cast<int>(m_out.size()), kSendFlags);
            if (sent > 0)
                m_out.erase(0, static_cast<size_t>(sent));
            else if (sent < 0 && wouldBlock())
                return; // tampon systeme plein : on reprendra plus tard
            else
                m_closed = true;
        }
    }

    bool Socket::poll(std::vector<nlohmann::json> &_out)
    {
        if (!valid() || m_closed)
            return false;
        flush();

        char buf[64 * 1024];
        for (;;)
        {
            int got = ::recv(static_cast<Native>(m_handle), buf, sizeof(buf), 0);
            if (got > 0)
                m_in.append(buf, static_cast<size_t>(got));
            else if (got < 0 && wouldBlock())
                break;
            else
            {
                m_closed = true; // 0 = fermeture propre, <0 = erreur
                break;
            }
        }

        // Decoupe les messages complets.
        size_t pos = 0;
        while (m_in.size() - pos >= 4)
        {
            const unsigned char *h = reinterpret_cast<const unsigned char *>(m_in.data() + pos);
            uint32_t n = h[0] | (h[1] << 8) | (h[2] << 16) | (static_cast<uint32_t>(h[3]) << 24);
            if (n > kMaxMessage)
            {
                std::fprintf(stderr, "[ipc] message trop gros (%u octets), connexion coupee\n", n);
                m_closed = true;
                break;
            }
            if (m_in.size() - pos - 4 < n)
                break; // message incomplet : on attend la suite
            try
            {
                _out.push_back(nlohmann::json::parse(m_in.begin() + pos + 4, m_in.begin() + pos + 4 + n));
            }
            catch (const std::exception &)
            {
                std::fprintf(stderr, "[ipc] message JSON invalide ignore\n");
            }
            pos += 4 + n;
        }
        m_in.erase(0, pos);
        return !m_closed;
    }

    void Socket::close()
    {
        if (valid())
            closeNative(static_cast<Native>(m_handle));
        m_handle = kInvalid;
        m_closed = true;
        m_in.clear();
        m_out.clear();
    }
}
