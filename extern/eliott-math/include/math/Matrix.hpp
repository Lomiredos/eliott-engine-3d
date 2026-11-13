#pragma once
#include <concepts>
#include <cstddef>

namespace ee::math {

    template<std::size_t N, std::floating_point T>
        requires(N >= 2)
    class Matrix{




        public:
            T m[N *N]{};

            [[nodiscard]] static constexpr Matrix identity() noexcept {
                Matrix mat{};
                for (std::size_t i = 0; i < N; ++i){
                    mat(i, i) = T(1);    
                }
                return mat;
            }
            [[nodiscard]] constexpr T& operator()(std::size_t _row, std::size_t _col) noexcept {
                return m[_col * N + _row];
            }
            [[nodiscard]] constexpr T operator()(std::size_t _row, std::size_t _col) const noexcept {
                return m[_col * N + _row];
            }

            [[nodiscard]] constexpr bool operator==(const Matrix&) const = default;
            [[nodiscard]] constexpr bool nearlyEqual(const Matrix& _o, T _eps = T(1e-5)) const noexcept {
                for (std::size_t i = 0; i < N * N; ++i)
                    if (T d = m[i] - _o.m[i]; !((d > 0 ? d : -d) <= _eps)) return false;
                return true;
            }

            [[nodiscard]] constexpr const T* data() const noexcept { return m; }
            [[nodiscard]] constexpr T* data() noexcept { return m; }

            [[nodiscard]] constexpr Matrix operator*(const Matrix& _o) const noexcept {
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

            [[nodiscard]] constexpr Matrix transposed() const noexcept {
                Matrix r{};
                for (std::size_t i = 0; i < N; ++i) {
                    for (std::size_t j = 0; j < N; ++j) {
                        r(i, j) = (*this)(j, i);
                    }
                }
                return r;
            }

            constexpr Matrix& operator+=(const Matrix& _o) noexcept {
                for (std::size_t i = 0; i < N * N; ++i) {
                    m[i] += _o.m[i];
                }
                return (*this);
            }

            constexpr Matrix& operator-=(const Matrix& _o) noexcept {
                for (std::size_t i = 0; i < N * N; ++i) {
                    m[i] -= _o.m[i];
                }
                return (*this);
            }

            constexpr Matrix& operator*=(T _t) noexcept {
                for (std::size_t i = 0; i < N * N; ++i) {
                    m[i] *= _t;
                }
                return (*this);
            }

            constexpr Matrix& operator*=(const Matrix& _o) noexcept {
                *this = *this * _o;
                return *this;
            }

            [[nodiscard]] constexpr Matrix operator+(const Matrix& _o) const noexcept {
                Matrix r = *this;
                r += _o;
                return r;
            }

            [[nodiscard]] constexpr Matrix operator-(const Matrix& _o) const noexcept {
                Matrix r = *this;
                r -= _o;
                return r;
            }

            [[nodiscard]] constexpr Matrix operator*(T _t) const noexcept {
                Matrix r = *this;
                r *= _t;
                return r;
            }

            [[nodiscard]] friend constexpr Matrix operator*(T _t, const Matrix& _m) noexcept { return _m * _t; }
    };


    
    
} //namespace