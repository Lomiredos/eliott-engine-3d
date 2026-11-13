#pragma once
#include <concepts>
#include <cstddef>

namespace ee::math {

    template<std::size_t N, std::floating_point T>
        requires(N >= 2)
    class Matrix{




        public:
            T m[N *N]{};


            static constexpr Matrix identity() {
                Matrix mat{};
                for (std::size_t i = 0; i < N; ++i){
                    mat(i, i) = T(1);    
                }
                return mat;
            }
            constexpr T& operator()(std::size_t _row, std::size_t _col) {return m[_col * N + _row];}
            constexpr const T operator()(std::size_t _row, std::size_t _col) const {return m[_col * N + _row];}
    
            constexpr bool operator==(const Matrix &) const = default;

            constexpr bool nearlyEqual(const Matrix &_o, T _eps = T(1e-5)) const {
                for (std::size_t i = 0; i < N * N; ++i)
                    if (T d = m[i] - _o.m[i]; !((d > 0 ? d : -d) <= _eps)) return false;
                return true;
            }



            constexpr const T* data() const {return m;}
            constexpr T* data() {return m;}
    };


    
    
} //namespace