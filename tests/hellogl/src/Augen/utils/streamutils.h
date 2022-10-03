#pragma once

#include <glm/gtx/string_cast.hpp>
#include <type_traits>
#include <utility>
#include <vector>
#include <iostream>

template <typename, typename = void>
struct can_cout : public std::false_type {};

template <typename T>
struct can_cout<T, std::void_t<decltype(operator<<(std::declval<std::ostream>(), std::declval<T>()))> > : public std::true_type {};

template <
    typename GlmType,
    typename = std::enable_if_t<!can_cout<GlmType>::value>,
    typename = decltype(glm::to_string(std::declval<GlmType>())),
    typename = decltype(glm::detail::compute_to_string<GlmType>::call(std::declval<GlmType>()))
>
std::ostream& operator<<(std::ostream& out, const GlmType& g)
{
    return out << glm::to_string(g);
}

template <
    typename T,
    typename = std::enable_if_t<can_cout<T>::value>
>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& vec)
{
    out << "[";
    bool first = true;
    for (const auto& x : vec) {
        if (!first) out << ", ";
        out << x;
        first = false;
    }
    out << "]";
    return out;
}

template <typename T>
std::ostream& print_array(std::ostream& out, const std::vector<T>& vec) {
    out << "[";
    bool first = true;
    for (const auto& x : vec) {
        if (!first) out << ", ";
        out << x;
        first = false;
    }
    out << "]";
    return out;
}
