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
            [[nodiscard]] static constexpr Matrix translation(const Vector3<T>& _t) noexcept
                requires(N == 4);
            [[nodiscard]] static constexpr Matrix scaling(const Vector3<T>& _s) noexcept
                requires(N == 4);
            [[nodiscard]] static Matrix rotation(const Quaternion& _q) noexcept
                requires(N == 4 || N == 3);
            [[nodiscard]] static Matrix fromTRS(const Vector3<T>& _t, const Quaternion& _r,
                                                const Vector3<T>& _s) noexcept
                requires(N == 4);
    };

} //namespace

#include "Matrix.inl"