//
// Copyright (c) 2014 Krzysztof Jusiak (krzysztof at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_DI_CORE_INJECTOR_HPP
#define BOOST_DI_CORE_INJECTOR_HPP

#include "boost/di/aux_/memory.hpp"
#include "boost/di/aux_/type_traits.hpp"
#include "boost/di/aux_/utility.hpp"
#include "boost/di/core/pool.hpp"
#include "boost/di/core/any_type.hpp"
#include "boost/di/core/binder.hpp"
#include "boost/di/providers/min_allocs.hpp"
#include "boost/di/wrappers/universal.hpp"
#include "boost/di/type_traits/ctor_traits.hpp"
#include "boost/di/scopes/exposed.hpp"

namespace boost {
namespace di {
namespace core {

BOOST_DI_HAS_TYPE(deps);
BOOST_DI_HAS_METHOD(configure, configure);
BOOST_DI_HAS_METHOD(call, call);

template<class T>
using is_injector = bool_<has_deps<T>{} || has_configure<T>{}>;

template<class T, class = void>
struct get_deps {
    using type = typename T::deps;
};

template<class T>
struct get_deps<T, aux::enable_if_t<has_configure<T>{}>> {
    using type = typename aux::function_traits<decltype(&T::configure)>::result_type::deps;
};

template<class T, class = typename is_injector<T>::type, class = typename is_dependency<T>::type>
struct add_type_list;

template<class T, class TAny>
struct add_type_list<T, std::true_type, TAny> {
    using type = typename get_deps<T>::type;
};

template<class T>
struct add_type_list<T, std::false_type, std::true_type> {
    using type = type_list<T>;
};

template<class T>
struct add_type_list<T, std::false_type, std::false_type> {
    using type = type_list<dependency<scopes::exposed, T>>;
};

template<class... Ts>
using bindings_t = typename join<typename add_type_list<Ts>::type...>::type;

template<class TDeps = type_list<>, typename TDefaultProvider = providers::min_allocs>
class injector : public pool<TDeps> {
    template<class, class, class, class, class>
    friend class any_type;

    using pool_t = pool<TDeps>;

    template<class T, class TDependency>
    struct data {
        using type = T;
        using dependency = TDependency;
        using binder = core::binder;
    };

    template<class...>
    struct provider_impl;

    template<
        class T
      , class TDependency
      , class TProvider
      , class TPolicies
      , class... TArgs
    >
    struct provider_impl<T, TDependency, TProvider, TPolicies, type_list<TArgs...>> {
        const injector& injector_;
        const TProvider& provider_;
        const TPolicies& policies_;

        decltype(auto) get() const noexcept {
            aux::shared_ptr<void> v;
            return provider_.template get<TDependency>(
                injector_.create_impl<TArgs, T>(provider_, v, policies_)...
            );
        }

        decltype(auto) get_ptr() const noexcept {
            aux::shared_ptr<void> v;
            return provider_.template get_ptr<TDependency>(
                injector_.create_impl<TArgs, T>(provider_, v, policies_)...
            );
        }
    };

public:
    using deps = TDeps;

    // bind<...>, etc.  -> ignore
    // injector<...>    -> get all dependencies from the injector
    // dependency<...>  -> pass

    template<class... TArgs>
    explicit injector(const TArgs&... args) noexcept
        : injector(init{}, pass_arg(args)...)
    { }

    template<class T>
    injector(const injector<T>& injector) noexcept // non explicit
        : pool_t{init{}, create_from_injector(injector, TDeps{})}
    { }

    template<class T, class... TArgs>
    T create(const TArgs&... args) const noexcept {
        pool<type_list<TArgs...>> policies(args...);
        aux::shared_ptr<void> v;
        return create_impl<T, none_t>(TDefaultProvider{}, v, policies);
    }

    template<class T, class TProvider, class... TArgs>
    T provide(const TProvider& provider, const TArgs&... args) const noexcept {
        pool<type_list<TArgs...>> policies(args...);
        aux::shared_ptr<void> v;
        return create_impl<T, none_t>(provider, v, policies);
    }

    template<class TAction>
    void call(const TAction& action) noexcept {
        call_impl(action, deps{});
    }

private:
    template<class... TArgs>
    injector(const init&, const TArgs&... args) noexcept
        : pool_t{init{}, pool<type_list<TArgs...>>{args...}}
    { }

    template<
        class T
      , class TParent
      , class TProvider
      , class TRefs
      , class TPolicies
    >
    decltype(auto)
    create_impl(const TProvider& provider, TRefs& refs, const TPolicies& policies
              , aux::enable_if_t<is_any_type<T>{}>* = 0) const noexcept {
        return any_type<TParent, injector, TProvider, TRefs, TPolicies>{
            *this, provider, refs, policies
        };
    }

    template<
        class T
      , class TParent
      , class TProvider
      , class TRefs
      , class TPolicies
    >
    decltype(auto)
    create_impl(const TProvider& provider, TRefs& refs, const TPolicies& policies
              , aux::enable_if_t<!is_any_type<T>{}>* = 0) const noexcept {
        return create_from_dep_impl<T>(provider, refs, policies);
    }

    template<
        class T
      , class TProvider
      , class TRefs
      , class TPolicies
    >
    decltype(auto)
    create_from_dep_impl(const TProvider& provider, TRefs& refs, const TPolicies& policies) const noexcept {
        const auto& dependency = binder::resolve<T>(this);
        using type = typename aux::remove_reference_t<decltype(dependency)>::given;;
        using ctor = typename type_traits::ctor_traits<type>::type;
        call_policies<data<T, type>>(policies);
        return wrappers::universal<T>{refs, dependency.template create<T>(
            provider_impl<T, type, TProvider, TPolicies, ctor>{*this, provider, policies})
        };
    }

    template<class T, class... TPolicies>
    static void call_policies(const pool<type_list<TPolicies...>>& policies) noexcept {
        void(call_policies_impl<TPolicies, T>(policies)...);
    }

    template<class TPolicy, class T, class TPolicies>
    static void call_policies_impl(const TPolicies& policies) noexcept {
        (static_cast<const TPolicy&>(policies))(T{});
    }

    template<class TAction, class... Ts>
    void call_impl(const TAction& action, const type_list<Ts...>&) noexcept {
        void(call_impl<Ts>(action)...);
    }

    template<class T, class TAction>
    aux::enable_if_t<has_call<T, const TAction&>{}>
    call_impl(const TAction& action) noexcept {
        static_cast<T&>(*this).call(action);
    }

    template<class T, class TAction>
    aux::enable_if_t<!has_call<T, const TAction&>{}>
    call_impl(const TAction&) noexcept { }

    template<class T>
    decltype(auto) pass_arg(const T& arg, aux::enable_if_t<!has_configure<T>{}>* = 0) const noexcept {
        return arg;
    }

    template<class T>
    decltype(auto) pass_arg(const T& arg, aux::enable_if_t<has_configure<T>{}>* = 0) const noexcept {
        return arg.configure();
    }

    template<class T, class... Ts>
    decltype(auto) create_from_injector(const injector<T>& injector, const type_list<Ts...>&) const noexcept {
        return pool<TDeps>(create_dep<Ts>(injector)...);
    }

    template<class TDependency, class TInjector>
    decltype(auto) create_dep(const TInjector& injector) const noexcept {
        struct provider {
            TInjector injector_;

            decltype(auto) get() const noexcept {
                return injector_.template create<typename TDependency::given*>();
            }
        };

        return TDependency{provider{injector}};
    }
};

} // namespace core
} // namespace di
} // namespace boost

#endif

