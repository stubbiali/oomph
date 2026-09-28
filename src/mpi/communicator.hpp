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

#include <oomph/context.hpp>

// paths relative to backend
#include <../communicator_base.hpp>
#include <../device_guard.hpp>
#include <context.hpp>
#include <request_queue.hpp>

namespace oomph
{
class communicator_impl : public communicator_base<communicator_impl>
{
  public:
    context_impl* m_context;
    request_queue m_send_reqs;
    request_queue m_recv_reqs;

    communicator_impl(context_impl* ctxt)
    : communicator_base(ctxt)
    , m_context(ctxt)
    {
    }

    auto& get_heap() noexcept { return m_context->get_heap(); }

    bool is_stream_aware() const noexcept { return false; }

    void start_group() {}
    void end_group() {}

    mpi_request send(context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst,
        tag_type tag, [[maybe_unused]] void* stream)
    {
        MPI_Request        r;
        const_device_guard dg(ptr);
        OOMPH_CHECK_MPI_RESULT(MPI_Isend(dg.data(), size, MPI_BYTE, dst, tag, mpi_comm(), &r));
        return {r};
    }

    mpi_request recv(context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src,
        tag_type tag, [[maybe_unused]] void* stream)
    {
        MPI_Request  r;
        device_guard dg(ptr);
        OOMPH_CHECK_MPI_RESULT(MPI_Irecv(dg.data(), size, MPI_BYTE, src, tag, mpi_comm(), &r));
        return {r};
    }

    send_request send(context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst,
        tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb,
        std::size_t* scheduled, void* stream)
    {
        auto req = send(ptr, size, dst, tag, stream);
        if (!has_reached_recursion_depth() && req.is_ready())
        {
            auto inc = recursion();
            cb(dst, tag);
            return {};
        }
        else
        {
            auto s = m_req_state_factory.make(m_context, this, scheduled, dst, tag,
                std::move(cb), req);
            s->create_self_ref();
            m_send_reqs.enqueue(s.get());
            return {std::move(s)};
        }
    }

    recv_request recv(context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src,
        tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb,
        std::size_t* scheduled, void* stream)
    {
        auto req = recv(ptr, size, src, tag, stream);
        if (!has_reached_recursion_depth() && req.is_ready())
        {
            auto inc = recursion();
            cb(src, tag);
            return {};
        }
        else
        {
            auto s = m_req_state_factory.make(m_context, this, scheduled, src, tag,
                std::move(cb), req);
            s->create_self_ref();
            m_recv_reqs.enqueue(s.get());
            return {std::move(s)};
        }
    }

    shared_recv_request shared_recv(context_impl::heap_type::pointer& ptr, std::size_t size,
        rank_type src, tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb,
        std::atomic<std::size_t>* scheduled, void* stream)
    {
        auto req = recv(ptr, size, src, tag, stream);
        if (!m_context->has_reached_recursion_depth() && req.is_ready())
        {
            auto inc = m_context->recursion();
            cb(src, tag);
            return {};
        }
        else
        {
            auto s = std::make_shared<detail::shared_request_state>(m_context, this, scheduled, src,
                tag, std::move(cb), req);
            s->create_self_ref();
            m_context->m_req_queue.enqueue(s.get());
            return {std::move(s)};
        }
    }

    void progress()
    {
        m_send_reqs.progress();
        m_recv_reqs.progress();
        m_context->progress();
    }

    bool cancel_recv(detail::request_state* s) { return m_recv_reqs.cancel(s); }
};

} // namespace oomph
