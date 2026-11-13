#pragma once
#include "math/Quaternion.hpp"
#include "math/Vector3.hpp"
#include <concepts>
#include <cstddef>

namespace ee::math {

    template<std::size_t N, std::floating_point T>
        requires(N >= 2)
    class Matrix{




        public:
            T m[N *N]{};

            [[nodiscard]] static constexpr Matrix identity() noexcept;

            [[nodiscard]] constexpr T& operator()(std::size_t _row, std::size_t _col) noexcept;
            [[nodiscard]] constexpr T operator()(std::size_t _row, std::size_t _col) const noexcept;

            [[nodiscard]] constexpr bool operator==(const Matrix&) const = default;
            [[nodiscard]] constexpr bool nearlyEqual(const Matrix& _o, T _eps = T(1e-5)) const noexcept;

            [[nodiscard]] constexpr const T* data() const noexcept { return m; }
            [[nodiscard]] constexpr T* data() noexcept { return m; }

            [[nodiscard]] constexpr Matrix operator*(const Matrix& _o) const noexcept;

            [[nodiscard]] constexpr Matrix transposed() const noexcept;

            constexpr Matrix& operator+=(const Matrix& _o) noexcept;
            constexpr Matrix& operator-=(const Matrix& _o) noexcept;
            constexpr Matrix& operator*=(T _t) noexcept;
            constexpr Matrix& operator*=(const Matrix& _o) noexcept;

            [[nodiscard]] constexpr Matrix operator+(const Matrix& _o) const noexcept;
            [[nodiscard]] constexpr Matrix operator-(const Matrix& _o) const noexcept;
            [[nodiscard]] constexpr Matrix operator*(T _t) const noexcept;
            [[nodiscard]] friend constexpr Matrix operator*(T _t, const Matrix& _m) noexcept { return _m * _t; }

            // fonction reserver pour les 4x4 (ou 3x3 pour rotation)
            // transformation monde
            [[nodiscard]] static constexpr Matrix translation(const Vector3<T>& _t) noexcept
                requires(N == 4);
            [[nodiscard]] static constexpr Matrix scaling(const Vector3<T>& _s) noexcept
                requires(N == 4);
            [[nodiscard]] static Matrix rotation(const Quaternion& _q) noexcept
                requires(N == 4 || N == 3);
            [[nodiscard]] static Matrix fromTRS(const Vector3<T>& _t, const Quaternion& _r,
                                                const Vector3<T>& _s) noexcept
                requires(N == 4);

            // fonction reserver pour les 4x4
            // camera
            [[nodiscard]] static constexpr Matrix perspective(T _fovYRad, T _aspect, T _near, T _far) noexcept
                requires(N == 4)
            {
                Matrix r{};
                T t = std::tan(_fovYRad * T(0.5));
                r(0, 0) = T(1) / (_aspect * t);
                r(1, 1) = T(1) / t;
                r(2, 2) = -(_far + _near) / (_far - _near);
                r(2, 3) = -(T(2) * _far * _near) / (_far - _near);
                r(3, 2) = -T(1);

                return r;
            }

            [[nodiscard]] static constexpr Matrix ortho(T _l, T _r, T _b, T _t, T _near, T _far) noexcept
                requires(N == 4)
            {
                Matrix r{};
                r(0, 0) = T(2) / (_r - _l);
                r(1, 1) = T(2) / (_t - _b);
                r(2, 2) = -T(2) / (_far - _near);

                r(0, 3) = -(_r + _l) / (_r - _l);
                r(1, 3) = -(_t + _b) / (_t - _b);
                r(2, 3) = -(_far + _near) / (_far - _near);

                r(3, 3) = T(1);

                return r;
            }

            [[nodiscard]] static constexpr Matrix lookAt(const Vector3<T>& _eye, const Vector3<T>& _center,
                                                         const Vector3<T>& _up) noexcept
                requires(N == 4)
            {
                Vector3<T> f = _center - _eye;
                f.Normalized();
                Vector3<T> s{f.y * _up.z - f.z * _up.y, f.z * _up.x - f.x * _up.z, f.x * _up.y - f.y * _up.x};
                s.Normalized();
                Vector3<T> u{s.y * f.z - s.z * f.y, s.z * f.x - s.x * f.z, s.x * f.y - s.y * f.x};

                Matrix r{};

                r(0, 0) = s.x;
                r(0, 1) = s.y;
                r(0, 2) = s.z;

                r(1, 0) = u.x;
                r(1, 1) = u.y;
                r(1, 2) = u.z;

                r(2, 0) = -f.x;
                r(2, 1) = -f.y;
                r(2, 2) = -f.z;

                r(0, 3) = -(s.x * _eye.x + s.y * _eye.y + s.z * _eye.z);
                r(1, 3) = -(u.x * _eye.x + u.y * _eye.y + u.z * _eye.z);
                r(2, 3) = (f.x * _eye.x + f.y * _eye.y + f.z * _eye.z);

                r(3, 3) = T(1);
                return r;
            }
    };

} //namespace

#include "Matrix.inl"