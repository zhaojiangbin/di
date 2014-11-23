// Copyright (c) 2014 Krzysztof Jusiak (krzysztof at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_DI_SCOPES_DEDUCE_HPP
#define BOOST_DI_SCOPES_DEDUCE_HPP

#include "boost/di/type_traits/scope_traits.hpp"

namespace boost {
namespace di {
namespace scopes {

/**
 * virtual scope -> converted to the other scope
 * accordingly to the given type using scope_traits
 */
class deduce {
public:
    static constexpr auto priority = 0; // 0 - lowest, N - highest

    template<typename T>
    struct rebind {
        using other = type_traits::scope_traits_t<T>;
    };

    template<typename>
    struct scope { };
};

} // namespace scopes
} // namespace di
} // namespace boost

#endif

