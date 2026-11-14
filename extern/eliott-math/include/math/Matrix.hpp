#pragma once
#include "math/Quaternion.hpp"
#include "math/Vector3.hpp"
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <optional>

namespace ee::math {

    template<std::size_t N, std::floating_point T>
        requires(N >= 2)
    class Matrix{
    private:
        [[nodiscard]] constexpr T detPivotGauss() const noexcept;

    public:
        T m[N * N]{};

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

        [[nodiscard]] constexpr Vector3<T> operator*(const Vector3<T>& _v) const noexcept
            requires(N == 3);

        [[nodiscard]] constexpr Quaternion toQuaternion() const noexcept
            requires(N == 3)
        {

            T dw = T(1) + (*this)(0, 0) + (*this)(1, 1) + (*this)(2, 2);
            T dx = T(1) + (*this)(0, 0) - (*this)(1, 1) - (*this)(2, 2);
            T dy = T(1) - (*this)(0, 0) + (*this)(1, 1) - (*this)(2, 2);
            T dz = T(1) - (*this)(0, 0) - (*this)(1, 1) + (*this)(2, 2);

            T max = std::max({dw, dx, dy, dz});

            Quaternion q;
            if (max == dw) {
                q.w = T(0.5) * std::sqrt(dw);

                T invqw4 = T(0.25) / q.w;

                q.x = ((*this)(2, 1) - (*this)(1, 2)) * invqw4;
                q.y = ((*this)(0, 2) - (*this)(2, 0)) * invqw4;
                q.z = ((*this)(1, 0) - (*this)(0, 1)) * invqw4;
            } else if (max == dx) {
                q.x = T(0.5) * std::sqrt(dx);

                T invqx4 = T(0.25) / q.x;

                q.w = ((*this)(2, 1) - (*this)(1, 2)) * invqx4;
                q.y = ((*this)(0, 1) + (*this)(1, 0)) * invqx4;
                q.z = ((*this)(0, 2) + (*this)(2, 0)) * invqx4;
            } else if (max == dy) {

                q.y = T(0.5) * std::sqrt(dy);

                T invqy4 = T(0.25) / q.y;

                q.w = ((*this)(0, 2) - (*this)(2, 0)) * invqy4;
                q.x = ((*this)(0, 1) + (*this)(1, 0)) * invqy4;
                q.z = ((*this)(1, 2) + (*this)(2, 1)) * invqy4;
            } else {

                q.z = T(0.5) * std::sqrt(dz);

                T invqz4 = T(0.25) / q.z;

                q.w = ((*this)(1, 0) - (*this)(0, 1)) * invqz4;
                q.x = ((*this)(0, 2) + (*this)(2, 0)) * invqz4;
                q.y = ((*this)(1, 2) + (*this)(2, 1)) * invqz4;
            }

            return q;
        }

        // fonction reserver pour les 4x4 (ou 3x3 pour rotation)
        // transformation monde
        [[nodiscard]] static constexpr Matrix translation(const Vector3<T>& _t) noexcept
            requires(N == 4);
        [[nodiscard]] static constexpr Matrix scaling(const Vector3<T>& _s) noexcept
            requires(N == 4);
        [[nodiscard]] static Matrix rotation(const Quaternion& _q) noexcept
            requires(N == 4 || N == 3);
        [[nodiscard]] static Matrix fromTRS(const Vector3<T>& _t, const Quaternion& _r, const Vector3<T>& _s) noexcept
            requires(N == 4);

        // fonction reserver pour les 4x4
        // camera
        [[nodiscard]] static constexpr Matrix perspective(T _fovYRad, T _aspect, T _near, T _far) noexcept
            requires(N == 4);
        [[nodiscard]] static constexpr Matrix ortho(T _l, T _r, T _b, T _t, T _near, T _far) noexcept
            requires(N == 4);
        [[nodiscard]] static constexpr Matrix lookAt(const Vector3<T>& _eye, const Vector3<T>& _center,
                                                     const Vector3<T>& _up) noexcept
            requires(N == 4);

        // fonction reserver pour les 4x4
        // transformation
        [[nodiscard]] constexpr Vector3<T> getTranslation() const noexcept
            requires(N == 4);
        [[nodiscard]] constexpr Vector3<T> transformPoint(const Vector3<T>& _p) const noexcept
            requires(N == 4);
        [[nodiscard]] constexpr Vector3<T> projectPoint(const Vector3<T>& _p) const noexcept
            requires(N == 4);
        [[nodiscard]] constexpr Vector3<T> transformDir(const Vector3<T>& _d) const noexcept
            requires(N == 4);
        [[nodiscard]] constexpr Matrix<3, T> upperLeft3() const noexcept
            requires(N == 4);

        [[nodiscard]] constexpr Matrix<3, T> normalMatrix() const noexcept
            requires(N == 4);

        [[nodiscard]] constexpr T determinant() const noexcept;

        [[nodiscard]] constexpr std::optional<Matrix> inverse(T _eps = T(1e-6)) const noexcept;

        [[nodiscard]] constexpr Matrix inverseAffine() const noexcept
            requires(N == 4);

        constexpr bool decompose(Vector3<T>& _t, Quaternion& _r, Vector3<T>& _s) const noexcept
            requires(N == 4)
        {
            Matrix<3, T> ul = upperLeft3();
            T sca[3] = {T(0), T(0), T(0)};
            T eps = 1e-5;
            for (std::size_t col = 0; col < 3; ++col) {
                for (std::size_t row = 0; row < 3; ++row)
                    sca[col] += ul(row, col) * ul(row, col);
                if (sca[col] < eps) return false;
            }

            _t.x = (*this)(0, 3);
            _t.y = (*this)(1, 3);
            _t.z = (*this)(2, 3);

            _s.x = std::sqrt(sca[0]);
            _s.y = std::sqrt(sca[1]);
            _s.z = std::sqrt(sca[2]);

            T invSca[3] = {T(1) / _s.x, T(1) / _s.y, T(1) / _s.z};

            for (std::size_t k = 0; k < 3; ++k) {
                for (std::size_t col = 0; col < 3; ++col) {
                    ul(k, col) *= invSca[col];
                }
            }

            if (ul.determinant() < 0) {
                _s.x = -_s.x;
                for (std::size_t row = 0; row < 3; ++row) {
                    ul(row, 0) *= -1;
                }
            }

            _r = ul.toQuaternion();

            return true;
        }
    };

} //namespace

#include "Matrix.inl"