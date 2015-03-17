//
// Copyright (c) 2014 Krzysztof Jusiak (krzysztof at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_DI_CORE_ANY_TYPE_HPP
#define BOOST_DI_CORE_ANY_TYPE_HPP

#include <memory>
#include "boost/di/aux_/type_traits.hpp"
#include "boost/di/core/binder.hpp"

namespace boost { namespace di { namespace core {

auto creatable_impl_(...) -> std::false_type { return {}; }

template<class T, class B, class C>
auto creatable_impl_(T&& t, B&&, C&&) -> aux::is_valid_expr<decltype(t.template create_impl<B, no_name, std::true_type, C>())> { return {}; }

template<class T, class B, class C>
constexpr auto creatable_() {
    return decltype(creatable_impl_(std::declval<T>(), std::declval<B>(), std::declval<C>())){};
}

BOOST_DI_HAS_TYPE(is_ref);

template<class T, class TParent>
struct is_not_same_impl {
    static constexpr auto value =
        std::is_same<aux::decay_t<T>, aux::decay_t<TParent>>::value ||
        std::is_base_of<aux::decay_t<TParent>, aux::decay_t<T>>::value;
};

template<class T, class TParent>
using is_not_same = std::enable_if_t<!is_not_same_impl<T, TParent>::value>;

template<class TParent = void, class TInjector = aux::none_t>
struct any_type {
    template<class T>
    struct is_ref_impl {
        static constexpr auto value =
            std::is_same<TInjector, aux::none_t>::value ||
            has_is_ref<
                std::remove_reference_t<decltype(binder::resolve<T>((TInjector*)nullptr))>
            >::value;
    };

    template<class T>
    using is_ref = std::enable_if_t<is_ref_impl<T>::value>;

    template<class T, class = is_not_same<T, TParent>, class = std::enable_if_t<creatable_<TInjector, T, std::true_type>()>>
    operator T() {
        return injector_.template create_impl<T>();
    }

    template<class T, class = is_not_same<T, TParent>, class = is_ref<T>, class = std::enable_if_t<creatable_<TInjector, T&, std::true_type>()>>
    operator T&() const {
        return injector_.template create_impl<T&>();
    }

#if !defined(__clang__)
    template<class T, class = is_not_same<T, TParent>, class = is_ref<T>, class = std::enable_if_t<creatable_<TInjector, T&&, std::true_type>()>>
    operator T&&() const {
        return injector_.template create_impl<T&&>();
    }
#endif

    template<class T, class = is_not_same<T, TParent>, class = is_ref<T>, class = std::enable_if_t<creatable_<TInjector, const T&, std::true_type>()>>
    operator const T&() const {
        return injector_.template create_impl<const T&>();
    }

    const TInjector& injector_;
};

template<class>
struct is_any_type : std::false_type { };

template<class... TArgs>
struct is_any_type<any_type<TArgs...>> : std::true_type { };

}}} // boost::di::core

#endif

