#include "visu/ipc/SharedFrame.hpp"

#include <atomic>
#include <cstring>
#include <new> // placement new de l'en-tete

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace ee::ipc
{
    // En tete du bloc, suivi des deux buffers RGBA.
    struct SharedFrame::Header
    {
        std::atomic<uint64_t> counter; // incremente a chaque image publiee
        std::atomic<uint32_t> ready;   // indice (0/1) du buffer publie
        std::atomic<uint32_t> width;
        std::atomic<uint32_t> height;
    };

    namespace
    {
        constexpr std::size_t kHeaderSize = 64; // garde les buffers alignes
        constexpr std::size_t kBufferSize =
            static_cast<std::size_t>(SharedFrame::kMaxWidth) * SharedFrame::kMaxHeight * 4;
        constexpr std::size_t kTotalSize = kHeaderSize + 2 * kBufferSize;
        static_assert(sizeof(std::atomic<uint64_t>) + 3 * sizeof(std::atomic<uint32_t>) <= kHeaderSize);

#ifdef _WIN32
        std::string winName(const std::string &_name) { return "Local\\" + _name; }
#else
        std::string posixName(const std::string &_name) { return "/" + _name; }
#endif
    }

    SharedFrame::~SharedFrame() { close(); }

    SharedFrame::Header *SharedFrame::header() const { return static_cast<Header *>(m_base); }

    uint8_t *SharedFrame::buffer(int _index) const
    {
        return static_cast<uint8_t *>(m_base) + kHeaderSize + static_cast<std::size_t>(_index) * kBufferSize;
    }

    bool SharedFrame::create(const std::string &_name)
    {
        close();
#ifdef _WIN32
        HANDLE h = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                      static_cast<DWORD>(static_cast<uint64_t>(kTotalSize) >> 32),
                                      static_cast<DWORD>(kTotalSize & 0xFFFFFFFFu),
                                      winName(_name).c_str());
        if (!h)
            return false;
        void *p = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, kTotalSize);
        if (!p)
        {
            CloseHandle(h);
            return false;
        }
        m_mapping = h;
#else
        std::string n = posixName(_name);
        shm_unlink(n.c_str()); // reste d'une session plantee
        int fd = shm_open(n.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
        if (fd < 0)
            return false;
        if (ftruncate(fd, static_cast<off_t>(kTotalSize)) != 0)
        {
            ::close(fd);
            shm_unlink(n.c_str());
            return false;
        }
        void *p = mmap(nullptr, kTotalSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        ::close(fd);
        if (p == MAP_FAILED)
        {
            shm_unlink(n.c_str());
            return false;
        }
#endif
        m_base = p;
        m_size = kTotalSize;
        m_name = _name;
        m_owner = true;

        // Le bloc est neuf (rempli de zeros) : on initialise l'en-tete en place.
        Header *hd = new (m_base) Header;
        hd->counter = 0;
        hd->ready = 0;
        hd->width = 0;
        hd->height = 0;
        return true;
    }

    bool SharedFrame::open(const std::string &_name)
    {
        close();
#ifdef _WIN32
        HANDLE h = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, winName(_name).c_str());
        if (!h)
            return false;
        void *p = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, kTotalSize);
        if (!p)
        {
            CloseHandle(h);
            return false;
        }
        m_mapping = h;
#else
        int fd = shm_open(posixName(_name).c_str(), O_RDWR, 0600);
        if (fd < 0)
            return false;
        void *p = mmap(nullptr, kTotalSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        ::close(fd);
        if (p == MAP_FAILED)
            return false;
#endif
        m_base = p;
        m_size = kTotalSize;
        m_name = _name;
        m_owner = false;
        return true;
    }

    void SharedFrame::close()
    {
        if (!m_base)
            return;
#ifdef _WIN32
        UnmapViewOfFile(m_base);
        CloseHandle(static_cast<HANDLE>(m_mapping));
        m_mapping = nullptr;
#else
        munmap(m_base, m_size);
        if (m_owner)
            shm_unlink(posixName(m_name).c_str());
#endif
        m_base = nullptr;
        m_size = 0;
        m_owner = false;
    }

    uint8_t *SharedFrame::backBuffer()
    {
        if (!m_base)
            return nullptr;
        return buffer(1 - static_cast<int>(header()->ready.load(std::memory_order_acquire)));
    }

    void SharedFrame::publish(int _w, int _h)
    {
        if (!m_base)
            return;
        Header *hd = header();
        uint32_t back = 1 - hd->ready.load(std::memory_order_acquire);
        hd->width.store(static_cast<uint32_t>(_w), std::memory_order_relaxed);
        hd->height.store(static_cast<uint32_t>(_h), std::memory_order_relaxed);
        hd->ready.store(back, std::memory_order_release);
        hd->counter.fetch_add(1, std::memory_order_release);
    }

    bool SharedFrame::latest(uint64_t &_lastCounter, const uint8_t *&_pixels, int &_w, int &_h) const
    {
        if (!m_base)
            return false;
        Header *hd = header();
        uint64_t c = hd->counter.load(std::memory_order_acquire);
        if (c == _lastCounter)
            return false;
        uint32_t idx = hd->ready.load(std::memory_order_acquire);
        _w = static_cast<int>(hd->width.load(std::memory_order_relaxed));
        _h = static_cast<int>(hd->height.load(std::memory_order_relaxed));
        if (_w <= 0 || _h <= 0)
            return false;
        _pixels = buffer(static_cast<int>(idx));
        _lastCounter = c;
        return true;
    }
}
