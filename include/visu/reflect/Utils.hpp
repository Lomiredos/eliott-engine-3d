#pragma once

#include <cstddef>
#include <utility>

namespace ee::reflection{

    template <typename V, std::size_t... Is>
    void emplaceByIndex(V& _v, std::size_t _i, std::index_sequence<Is...>){
        ((_i == Is ? (void)_v.template emplace<Is>() : (void)0), ...);
    }
}