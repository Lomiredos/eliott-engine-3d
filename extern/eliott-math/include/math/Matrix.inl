#pragma once
#include "Matrix.hpp"
#include "math/Quaternion.hpp"
#include <concepts>
#include <cstddef>

namespace ee::math {

    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr T Matrix<N, T>::detPivotGauss() const noexcept {
        Matrix a = *this;
        int signe = 1;

        auto switchline = [&a](std::size_t _a, std::size_t _b) {
            for (std::size_t i = 0; i < N; ++i) {
                std::swap(a(_a, i), a(_b, i));
            }
        };

        auto substractline = [&a](std::size_t _a, std::size_t _b, T _k, std::size_t _c) {
            for (std::size_t i = _c; i < N; i++) {
                a(_a, i) -= a(_b, i) * _k;
            }
        };

        for (std::size_t col = 0; col < N - 1; ++col) {
            T max = a(col, col);
            if (max < T(0)) max *= T(-1);
            std::size_t p = col;
            for (std::size_t l = col; l < N; ++l) {
                if (T v = a(l, col) > T(0) ? a(l, col) : -a(l, col); v > max) {
                    max = v;
                    p = l;
                }
            }

            if (max <= T(0)) return 0;

            if (p != col) {
                signe = -signe;
                switchline(p, col);
            }

            for (std::size_t row = col + 1; row < N; ++row) {
                T k = a(row, col) / a(col, col);
                substractline(row, col, k, col);
            }
        }

        T det = signe;
        for (std::size_t i = 0; i < N; ++i) {
            det *= a(i, i);
        }
        return det;
    }

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
    constexpr Vector3<T> Matrix<N, T>::operator*(const Vector3<T>& _v) const noexcept
        requires(N == 3)
    {
        const T b[3]{_v.x, _v.y, _v.z};
        T r[3]{};

        for (std::size_t k = 0; k < 3; ++k) {
            for (std::size_t row = 0; row < 3; ++row) {
                r[row] += (*this)(row, k) * b[k];
            }
        }
        return {r[0], r[1], r[2]};
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

    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Matrix<N, T> Matrix<N, T>::perspective(T _fovYRad, T _aspect, T _near, T _far) noexcept
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

    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Matrix<N, T> Matrix<N, T>::ortho(T _l, T _r, T _b, T _t, T _near, T _far) noexcept
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

    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Matrix<N, T> Matrix<N, T>::lookAt(const Vector3<T>& _eye, const Vector3<T>& _center,
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

    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Vector3<T> Matrix<N, T>::getTranslation() const noexcept
        requires(N == 4)
    {
        return {(*this)(0, 3), (*this)(1, 3), (*this)(2, 3)};
    }

    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Vector3<T> Matrix<N, T>::transformPoint(const Vector3<T>& _p) const noexcept
        requires(N == 4)
    {
        const T b[3]{_p.x, _p.y, _p.z};
        Vector3<T> trans = getTranslation();
        T r[3]{trans.x, trans.y, trans.z};

        for (std::size_t k = 0; k < 3; ++k) {
            for (std::size_t row = 0; row < 3; ++row) {
                r[row] += (*this)(row, k) * b[k];
            }
        }
        return {r[0], r[1], r[2]};
    }
    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Vector3<T> Matrix<N, T>::projectPoint(const Vector3<T>& _p) const noexcept
        requires(N == 4)
    {
        const T b[4]{_p.x, _p.y, _p.z, T(1)};
        T r[4]{};

        for (std::size_t k = 0; k < 4; ++k) {
            for (std::size_t row = 0; row < 4; ++row) {
                r[row] += (*this)(row, k) * b[k];
            }
        }
        if (r[3] == T(0)) return {r[0], r[1], r[2]};
        const T inv = T(1) / r[3];
        return {r[0] * inv, r[1] * inv, r[2] * inv}; // 1 div + 3 mult coute moin cher que 3 div
    }
    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Vector3<T> Matrix<N, T>::transformDir(const Vector3<T>& _d) const noexcept
        requires(N == 4)
    {
        const T b[3]{_d.x, _d.y, _d.z};
        T r[3]{};

        for (std::size_t k = 0; k < 3; ++k) {
            for (std::size_t row = 0; row < 3; ++row) {
                r[row] += (*this)(row, k) * b[k];
            }
        }
        return {r[0], r[1], r[2]};
    }
    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Matrix<3, T> Matrix<N, T>::upperLeft3() const noexcept
        requires(N == 4)
    {
        Matrix<3, T> r{};

        for (std::size_t row = 0; row < 3; ++row) {
            for (std::size_t col = 0; col < 3; ++col) {
                r(row, col) = (*this)(row, col);
            }
        }
        return r;
    }

    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Matrix<3, T> Matrix<N, T>::normalMatrix() const noexcept
        requires(N == 4)
    {

        Matrix<3, T> ul = upperLeft3();
        Matrix<3, T> r{};

        const Vector3<T> c0{ul(0, 0), ul(1, 0), ul(2, 0)};
        const Vector3<T> c1{ul(0, 1), ul(1, 1), ul(2, 1)};
        const Vector3<T> c2{ul(0, 2), ul(1, 2), ul(2, 2)};

        const Vector3<T> n0 = c1.Cross(c2);
        const Vector3<T> n1 = c2.Cross(c0);
        const Vector3<T> n2 = c0.Cross(c1);

        r(0, 0) = n0.x;
        r(1, 0) = n0.y;
        r(2, 0) = n0.z;

        r(0, 1) = n1.x;
        r(1, 1) = n1.y;
        r(2, 1) = n1.z;

        r(0, 2) = n2.x;
        r(1, 2) = n2.y;
        r(2, 2) = n2.z;

        return r;
    }

    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr T Matrix<N, T>::determinant() const noexcept {
        if constexpr (N == 2) {
            return (*this)(0, 0) * (*this)(1, 1) - (*this)(0, 1) * (*this)(1, 0);
        } else if constexpr (N == 3) {
            return (*this)(0, 0) * (*this)(1, 1) * (*this)(2, 2) + (*this)(0, 1) * (*this)(1, 2) * (*this)(2, 0) +
                   (*this)(0, 2) * (*this)(1, 0) * (*this)(2, 1) - (*this)(2, 0) * (*this)(1, 1) * (*this)(0, 2) -
                   (*this)(2, 1) * (*this)(1, 2) * (*this)(0, 0) - (*this)(2, 2) * (*this)(1, 0) * (*this)(0, 1);
        } else {
            return detPivotGauss();
        }
    }

    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr std::optional<Matrix<N, T>> Matrix<N, T>::inverse(T _eps) const noexcept {
        auto absT = [](T _x) { return _x < T(0) ? -_x : _x; };

        if constexpr (N == 2) {
            const T det = determinant();
            if (absT(det) < _eps) return std::nullopt;

            const T invDet = T(1) / det;
            Matrix r{};
            r(0, 0) = (*this)(1, 1) * invDet;
            r(1, 1) = (*this)(0, 0) * invDet;
            r(0, 1) = -(*this)(0, 1) * invDet;
            r(1, 0) = -(*this)(1, 0) * invDet;
            return r;
        } else if constexpr (N == 3) {
            const Vector3<T> c0{(*this)(0, 0), (*this)(1, 0), (*this)(2, 0)};
            const Vector3<T> c1{(*this)(0, 1), (*this)(1, 1), (*this)(2, 1)};
            const Vector3<T> c2{(*this)(0, 2), (*this)(1, 2), (*this)(2, 2)};

            const Vector3<T> n0 = c1.Cross(c2);
            const Vector3<T> n1 = c2.Cross(c0);
            const Vector3<T> n2 = c0.Cross(c1);

            const T det = c0.x * n0.x + c0.y * n0.y + c0.z * n0.z;
            if (absT(det) < _eps) return std::nullopt;

            const T invDet = T(1) / det;
            const Vector3<T> rows[3]{n0, n1, n2};
            Matrix r{};
            for (std::size_t row = 0; row < 3; ++row) {
                r(row, 0) = rows[row].x * invDet;
                r(row, 1) = rows[row].y * invDet;
                r(row, 2) = rows[row].z * invDet;
            }
            return r;
        } else {
            Matrix a = *this;
            Matrix inv = identity();

            for (std::size_t col = 0; col < N; ++col) {
                std::size_t p = col;
                T max = absT(a(col, col));
                for (std::size_t row = col + 1; row < N; ++row) {
                    if (const T v = absT(a(row, col)); v > max) {
                        max = v;
                        p = row;
                    }
                }
                if (max < _eps) return std::nullopt;

                if (p != col) {
                    for (std::size_t i = 0; i < N; ++i) {
                        std::swap(a(p, i), a(col, i));
                        std::swap(inv(p, i), inv(col, i));
                    }
                }

                const T invPivot = T(1) / a(col, col);
                for (std::size_t i = 0; i < N; ++i) {
                    a(col, i) *= invPivot;
                    inv(col, i) *= invPivot;
                }

                for (std::size_t row = 0; row < N; ++row) {
                    if (row == col) continue;
                    const T k = a(row, col);
                    if (k == T(0)) continue;
                    for (std::size_t i = 0; i < N; ++i) {
                        a(row, i) -= k * a(col, i);
                        inv(row, i) -= k * inv(col, i);
                    }
                }
            }
            return inv;
        }
    }

    template <std::size_t N, std::floating_point T>
        requires(N >= 2)
    constexpr Matrix<N, T> Matrix<N, T>::inverseAffine() const noexcept
        requires(N == 4)
    {
        const std::optional<Matrix<3, T>> inv3 = upperLeft3().inverse();
        if (!inv3) return identity();

        const Vector3<T> t = *inv3 * getTranslation();

        Matrix r = identity();
        for (std::size_t col = 0; col < 3; ++col) {
            for (std::size_t row = 0; row < 3; ++row) {
                r(row, col) = (*inv3)(row, col);
            }
        }
        r(0, 3) = -t.x;
        r(1, 3) = -t.y;
        r(2, 3) = -t.z;
        return r;
    }
} // namespace
