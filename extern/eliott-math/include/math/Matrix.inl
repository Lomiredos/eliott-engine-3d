#pragma once
#include "Matrix.hpp"
#include "math/Quaternion.hpp"
#include <concepts>
#include <cstddef>

namespace ee::math {

    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Matrix<N, T> Matrix<N, T>::identity() noexcept {
        Matrix mat{};
        for (std::size_t i = 0; i < N; ++i) {
            mat(i, i) = T(1);
        }
        return mat;
    }


    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Matrix<N, T> Matrix<N, T>::operator*(const Matrix& _o) const noexcept {
        Matrix r{};
        for (std::size_t c = 0; c < N; ++c) {
            for (std::size_t k = 0; k < N; ++k) {
                const T b = _o(k, c);

                for (std::size_t row = 0; row < N; ++row) {
                    r(row, c) += (*this)(row, k) * b;
                }
            }
        }
        return r;
    }
    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Matrix<N, T> Matrix<N, T>::transposed() const noexcept {
        Matrix r{};
        for (std::size_t i = 0; i < N; ++i) {
            for (std::size_t j = 0; j < N; ++j) {
                r(i, j) = (*this)(j, i);
            }
        }
        return r;
    }
    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Matrix<N, T>& Matrix<N, T>::operator+=(const Matrix& _o) noexcept {
        for (std::size_t i = 0; i < N * N; ++i) {
            m[i] += _o.m[i];
        }
        return (*this);
    }
    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Matrix<N, T>& Matrix<N, T>::operator-=(const Matrix& _o) noexcept {
        for (std::size_t i = 0; i < N * N; ++i) {
            m[i] -= _o.m[i];
        }
        return (*this);
    }
    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Matrix<N, T>& Matrix<N, T>::operator*=(T _t) noexcept {
        for (std::size_t i = 0; i < N * N; ++i) {
            m[i] *= _t;
        }
        return (*this);
    }
    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Matrix<N, T>& Matrix<N, T>::operator*=(const Matrix& _o) noexcept {
        *this = *this * _o;
        return *this;
    }
    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Matrix<N, T> Matrix<N, T>::operator+(const Matrix& _o) const noexcept {
        Matrix r = *this;
        r += _o;
        return r;
    }
    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Matrix<N, T> Matrix<N, T>::operator-(const Matrix& _o) const noexcept {
        Matrix r = *this;
        r -= _o;
        return r;
    }
    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Matrix<N, T> Matrix<N, T>::operator*(T _t) const noexcept {
        Matrix r = *this;
        r *= _t;
        return r;
    }

    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr bool Matrix<N, T>::nearlyEqual(const Matrix& _o, T _eps) const noexcept {
        for (std::size_t i = 0; i < N * N; ++i)
            if (T d = m[i] - _o.m[i]; !((d > 0 ? d : -d) <= _eps)) return false;
        return true;
    }

    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr T Matrix<N, T>::operator()(std::size_t _row, std::size_t _col) const noexcept {
        return m[_col * N + _row];
    }

    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr T& Matrix<N, T>::operator()(std::size_t _row, std::size_t _col) noexcept {
        return m[_col * N + _row];
    }


    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Matrix<N, T> Matrix<N, T>::scaling(const Vector3<T>& _s) noexcept
        requires(N == 4)
    {
        Matrix r = Matrix::identity();
        r(0, 0) = _s.x;
        r(1, 1) = _s.y;
        r(2, 2) = _s.z;
        return r;
    }

    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Matrix<N, T> Matrix<N, T>::translation(const Vector3<T>& _t) noexcept
        requires(N == 4)
    {
        Matrix r = Matrix::identity();
        r(0, 3) = _t.x;
        r(1, 3) = _t.y;
        r(2, 3) = _t.z;
        return r;
    }
    template <std::size_t N, std::floating_point T>
                requires(N >= 2)
    Matrix<N, T> Matrix<N, T>::rotation(const Quaternion& _q) noexcept
        requires(N == 4 || N == 3)
    {
        Quaternion q = _q;
        if (_q.isNormalized() == false) q = _q.Normalize();
        // check formule de Rodrigues si tu comprend pas
        T w = static_cast<T>(q.w), x = static_cast<T>(q.x), y = static_cast<T>(q.y), z = static_cast<T>(q.z);
        Matrix r = identity();

        // pratique, les case symetrique on le meme produit avec le signe du w inverser !
        r(0, 0) = T(1) - T(2) * (y * y + z * z);
        r(1, 0) = T(2) * (x * y + w * z);
        r(2, 0) = T(2) * (x * z - w * y);
        r(0, 1) = T(2) * (x * y - w * z);
        r(1, 1) = T(1) - T(2) * (x * x + z * z);
        r(2, 1) = T(2) * (y * z + w * x);
        r(0, 2) = T(2) * (x * z + w * y);
        r(1, 2) = T(2) * (y * z - w * x);
        r(2, 2) = T(1) - T(2) * (x * x + y * y);

        return r;
    }

    template<std::size_t N, std::floating_point T>
        requires(N >= 2)
    Matrix<N, T> Matrix<N, T>::fromTRS(const Vector3<T> &_t, const Quaternion &_r, const Vector3<T> &_s) noexcept
        requires(N == 4)
    {
        Matrix r = rotation(_r);
        for (std::size_t row = 0; row < 3; ++row) {
            r(row, 0) *= _s.x;
            r(row, 1) *= _s.y;
            r(row, 2) *= _s.z;
        }

        r(0, 3) = _t.x;
        r(1, 3) = _t.y;
        r(2, 3) = _t.z;

        return r;
    }

} // namespace
