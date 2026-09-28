/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <atomic>
#include <type_traits>
#include <vector>

#include <oomph/message_buffer.hpp>
#include <oomph/request.hpp>
#include <oomph/types.hpp>
#include <oomph/util/pool_factory.hpp>

namespace oomph
{
namespace detail
{

template <typename CallBack, typename T>
using is_callback_for =
    std::is_invocable<CallBack, oomph::message_buffer<T>, rank_type, tag_type>;

template <typename CallBack, typename T>
using is_callback_for_ref = std::disjunction<
    std::is_invocable<CallBack, oomph::message_buffer<T>&, rank_type, tag_type>,
    std::is_invocable<CallBack, oomph::message_buffer<T> const&, rank_type, tag_type>>;

template <typename CallBack, typename T>
using is_callback_for_const_ref =
    std::is_invocable<CallBack, oomph::message_buffer<T> const&, rank_type, tag_type>;

template <typename CallBack, typename T>
using is_callback_multi_for =
    std::is_invocable<CallBack, oomph::message_buffer<T>, std::vector<rank_type>, tag_type>;

template <typename CallBack, typename T>
using is_callback_multi_for_ref = std::disjunction<
    std::is_invocable<CallBack, oomph::message_buffer<T>&, std::vector<rank_type>, tag_type>,
    std::is_invocable<CallBack, oomph::message_buffer<T> const&, std::vector<rank_type>, tag_type>>;

template <typename CallBack, typename T>
using is_callback_multi_for_const_ref =
    std::is_invocable<CallBack, oomph::message_buffer<T> const&, std::vector<rank_type>, tag_type>;

template <typename CallBack, typename T>
using is_callback_multi_tags_for =
    std::is_invocable<CallBack, oomph::message_buffer<T>, std::vector<rank_type>,
        std::vector<tag_type>>;

template <typename CallBack, typename T>
using is_callback_multi_tags_for_ref = std::disjunction<
    std::is_invocable<CallBack, oomph::message_buffer<T>&, std::vector<rank_type>,
        std::vector<tag_type>>,
    std::is_invocable<CallBack, oomph::message_buffer<T> const&, std::vector<rank_type>,
        std::vector<tag_type>>>;

template <typename CallBack, typename T>
using is_callback_multi_tags_for_const_ref =
    std::is_invocable<CallBack, oomph::message_buffer<T> const&, std::vector<rank_type>,
        std::vector<tag_type>>;

template <typename CallBack, typename T>
inline constexpr bool is_callback_for_v = is_callback_for<CallBack, T>::value;

template <typename CallBack, typename T>
inline constexpr bool is_callback_for_ref_v = is_callback_for_ref<CallBack, T>::value;

template <typename CallBack, typename T>
inline constexpr bool is_callback_for_const_ref_v = is_callback_for_const_ref<CallBack, T>::value;

template <typename CallBack, typename T>
inline constexpr bool is_callback_multi_for_v = is_callback_multi_for<CallBack, T>::value;

template <typename CallBack, typename T>
inline constexpr bool is_callback_multi_for_ref_v = is_callback_multi_for_ref<CallBack, T>::value;

template <typename CallBack, typename T>
inline constexpr bool is_callback_multi_for_const_ref_v =
    is_callback_multi_for_const_ref<CallBack, T>::value;

template <typename CallBack, typename T>
inline constexpr bool is_callback_multi_tags_for_v = is_callback_multi_tags_for<CallBack, T>::value;

template <typename CallBack, typename T>
inline constexpr bool is_callback_multi_tags_for_ref_v =
    is_callback_multi_tags_for_ref<CallBack, T>::value;

template <typename CallBack, typename T>
inline constexpr bool is_callback_multi_tags_for_const_ref_v =
    is_callback_multi_tags_for_const_ref<CallBack, T>::value;

} // namespace detail

class communicator_impl;

namespace detail
{
struct communicator_state
{
    using impl_type = communicator_impl;
    impl_type*                              m_impl;
    std::atomic<std::size_t>*               m_shared_scheduled_recvs;
    util::pool_factory<multi_request_state> m_mrs_factory;
    std::size_t                             scheduled_sends = 0;
    std::size_t                             scheduled_recvs = 0;

    communicator_state(impl_type* impl_, std::atomic<std::size_t>* shared_scheduled_recvs);
    ~communicator_state();
    communicator_state(communicator_state const&) = delete;
    communicator_state(communicator_state&&) = delete;
    communicator_state& operator=(communicator_state const&) = delete;
    communicator_state& operator=(communicator_state&&) = delete;

    auto make_multi_request_state(std::size_t ns) { return m_mrs_factory.make(m_impl, ns); }

    template<typename T>
    auto make_multi_request_state(std::vector<rank_type>&& neighs,
        oomph::message_buffer<T> const&                    msg)
    {
        return m_mrs_factory.make(m_impl, neighs.size(), std::move(neighs), std::vector<tag_type>{},
            msg.size(), &msg);
    }

    template<typename T>
    auto make_multi_request_state(std::vector<rank_type>&& neighs, std::vector<tag_type>&& tags,
        oomph::message_buffer<T> const& msg)
    {
        return m_mrs_factory.make(m_impl, neighs.size(), std::move(neighs), std::move(tags),
            msg.size(), &msg);
    }

    template<typename T>
    auto make_multi_request_state(std::vector<rank_type>&& neighs, oomph::message_buffer<T>& msg)
    {
        return m_mrs_factory.make(m_impl, neighs.size(), std::move(neighs), std::vector<tag_type>{},
            msg.size(), &msg);
    }

    template<typename T>
    auto make_multi_request_state(std::vector<rank_type>&& neighs, std::vector<tag_type>&& tags,
        oomph::message_buffer<T>& msg)
    {
        return m_mrs_factory.make(m_impl, neighs.size(), std::move(neighs), std::move(tags),
            msg.size(), &msg);
    }

    template<typename T>
    auto make_multi_request_state(std::vector<rank_type>&& neighs, oomph::message_buffer<T>&& msg)
    {
        return m_mrs_factory.make(m_impl, neighs.size(), std::move(neighs), std::vector<tag_type>{},
            msg.size(), nullptr, std::move(msg.m));
    }

    template<typename T>
    auto make_multi_request_state(std::vector<rank_type>&& neighs, std::vector<tag_type>&& tags,
        oomph::message_buffer<T>&& msg)
    {
        return m_mrs_factory.make(m_impl, neighs.size(), std::move(neighs), std::move(tags),
            msg.size(), nullptr, std::move(msg.m));
    }
};

} // namespace detail
} // namespace oomph
