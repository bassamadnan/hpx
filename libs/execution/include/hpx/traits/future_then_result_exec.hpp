//  Copyright (c) 2007-2017 Hartmut Kaiser
//  Copyright (c) 2013 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution/traits/is_executor.hpp>
#include <hpx/functional/result_of.hpp>
#include <hpx/traits/future_then_result.hpp>
#include <hpx/traits/future_traits.hpp>
#include <hpx/traits/is_future.hpp>
#include <hpx/type_support/always_void.hpp>
#include <hpx/type_support/identity.hpp>
#include <hpx/type_support/lazy_conditional.hpp>
#include <hpx/type_support/pack.hpp>

#include <type_traits>
#include <utility>

namespace hpx { namespace traits {
    ///////////////////////////////////////////////////////////////////////////
    namespace detail {
        template <typename Executor, typename T, typename Ts,
            typename Enable = void>
        struct executor_future;

        template <typename Executor, typename T, typename Enable = void>
        struct exposes_future_type : std::false_type
        {
        };

        template <typename Executor, typename T>
        struct exposes_future_type<Executor, T,
            typename hpx::util::always_void<
                typename Executor::template future_type<T>>::type>
          : std::true_type
        {
        };

        template <typename Executor, typename T, typename... Ts>
        struct executor_future<Executor, T, hpx::util::pack<Ts...>,
            typename std::enable_if<
                hpx::traits::is_two_way_executor<Executor>::value &&
                exposes_future_type<Executor, T>::value>::type>
        {
            using type = typename Executor::template future_type<T>;
        };

        template <typename Executor, typename T, typename... Ts>
        struct executor_future<Executor, T, hpx::util::pack<Ts...>,
            typename std::enable_if<
                hpx::traits::is_two_way_executor<Executor>::value &&
                !exposes_future_type<Executor, T>::value>::type>
        {
            using type = decltype(std::declval<Executor&&>().async_execute(
                std::declval<T (*)(Ts...)>(), std::declval<Ts>()...));
        };

        template <typename Executor, typename T, typename Ts>
        struct executor_future<Executor, T, Ts,
            typename std::enable_if<
                !hpx::traits::is_two_way_executor<Executor>::value>::type>
        {
            using type = hpx::lcos::future<T>;
        };
    }    // namespace detail

    template <typename Executor, typename T, typename... Ts>
    struct executor_future
      : detail::executor_future<typename std::decay<Executor>::type, T,
            hpx::util::pack<typename std::decay<Ts>::type...>>
    {
    };

    template <typename Executor, typename T, typename... Ts>
    using executor_future_t =
        typename executor_future<Executor, T, Ts...>::type;

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {
        ///////////////////////////////////////////////////////////////////////
        template <typename Executor, typename Future, typename F,
            typename Enable = void>
        struct future_then_executor_result
        {
            typedef typename continuation_not_callable<Future, F>::type type;
        };

        template <typename Executor, typename Future, typename F>
        struct future_then_executor_result<Executor, Future, F,
            typename hpx::util::always_void<
                typename hpx::util::invoke_result<F&, Future>::type>::type>
        {
            typedef typename hpx::util::invoke_result<F&, Future>::type
                func_result_type;

            typedef typename traits::executor_future<Executor, func_result_type,
                Future>::type cont_result;

            // perform unwrapping of future<future<R>>
            typedef typename util::lazy_conditional<
                hpx::traits::detail::is_unique_future<cont_result>::value,
                hpx::traits::future_traits<cont_result>,
                hpx::util::identity<cont_result>>::type result_type;

            typedef hpx::lcos::future<result_type> type;
        };
    }    // namespace detail

    template <typename Executor, typename Future, typename F>
    struct future_then_executor_result
      : detail::future_then_executor_result<typename std::decay<Executor>::type,
            Future, F>
    {
    };
}}    // namespace hpx::traits
