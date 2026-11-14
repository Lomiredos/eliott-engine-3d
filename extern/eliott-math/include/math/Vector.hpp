#pragma once


#include <concepts>

template<typename T>
concept Arithmetic = (std::integral<T> || std::floating_point<T>) && !std::same_as<T, bool>;


template<std::size_t N, Arithmetic T>
    requires (N >= 2 && N <= 4)
struct Vector;