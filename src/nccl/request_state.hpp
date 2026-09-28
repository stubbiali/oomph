/*
 * ghex-org
 *
 * Copyright (c) 2014-2026, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <oomph/request.hpp>

// paths relative to backend
#include "../request_state_base.hpp"
#include "request.hpp"

namespace oomph::detail
{
struct request_state
: public util::enable_shared_from_this<request_state>
, public request_state_base<false>
{
    using base = request_state_base<false>;
    using shared_ptr_t = util::unsafe_shared_ptr<request_state>;

    nccl_request m_req;
    shared_ptr_t m_self_ptr;

    request_state(oomph::context_impl* ctxt, oomph::communicator_impl* comm, std::size_t* scheduled,
        rank_type rank, tag_type tag, cb_type&& cb, nccl_request m)
    : base{ctxt, comm, scheduled, rank, tag, std::move(cb)}
    , m_req{std::move(m)}
    {
    }

    void progress();

    bool cancel();

    void create_self_ref()
    {
        // create a self-reference cycle!!
        // this is useful if we only keep a raw pointer around internally, which still is supposed
        // to keep the object alive
        m_self_ptr = shared_from_this();
    }

    shared_ptr_t release_self_ref() noexcept
    {
        assert(((bool)m_self_ptr) && "doesn't own a self-reference!");
        return std::move(m_self_ptr);
    }
};

struct shared_request_state
: public std::enable_shared_from_this<shared_request_state>
, public request_state_base<true>
{
    using base = request_state_base<true>;
    using shared_ptr_t = std::shared_ptr<shared_request_state>;

    nccl_request m_req;
    shared_ptr_t m_self_ptr;

    shared_request_state(oomph::context_impl* ctxt, oomph::communicator_impl* comm,
        std::atomic<std::size_t>* scheduled, rank_type rank, tag_type tag, cb_type&& cb,
        nccl_request m)
    : base{ctxt, comm, scheduled, rank, tag, std::move(cb)}
    , m_req{std::move(m)}
    {
    }

    void progress();

    bool cancel();

    void create_self_ref()
    {
        // create a self-reference cycle!!
        // this is useful if we only keep a raw pointer around internally, which still is supposed
        // to keep the object alive
        m_self_ptr = shared_from_this();
    }

    shared_ptr_t release_self_ref() noexcept
    {
        assert(((bool)m_self_ptr) && "doesn't own a self-reference!");
        return std::move(m_self_ptr);
    }
};
} // namespace oomph::detail
