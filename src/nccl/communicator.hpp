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

#include <chrono>
#include <optional>
#include <stdexcept>
#include <thread>

#include <nccl.h>

#include <oomph/context.hpp>

// paths relative to backend
#include "../communicator_base.hpp"
#include "../device_guard.hpp"
#include "./context.hpp"
#include "cached_cuda_event.hpp"
#include "group_cuda_event.hpp"
#include "request.hpp"
#include "request_queue.hpp"
#include "request_state.hpp"

namespace oomph
{
class communicator_impl : public communicator_base<communicator_impl>
{
  public:
    context_impl* m_context;
    request_queue m_send_reqs;
    request_queue m_recv_reqs;

  private:
    struct group_info
    {
        // A shared CUDA event used for synchronization at the end of the NCCL
        // group. All streams used within the group are waited for before the
        // group kernel starts and all streams can be used to wait for the
        // completion of the group kernel. From
        // https://docs.nvidia.com/deeplearning/nccl/user-guide/docs/usage/streams.html:
        //
        // NCCL allows for using multiple streams within a group call. This will
        // enforce a stream dependency of all streams before the NCCL kernel
        // starts and block all streams until the NCCL kernel completes.
        //
        // It will behave as if the NCCL group operation was posted on every
        // stream, but given it is a single operation, it will cause a global
        // synchronization point between the streams.
        detail::group_cuda_event m_event{};

        // We arbitrarily use the last stream used within a group to synchronize
        // the whole group. nullopt indicates no operations were posted.
        std::optional<cudaStream_t> m_last_stream;

        bool has_operations() const noexcept { return m_last_stream.has_value(); }
    };

    // NCCL group information. When no group is active this is std::nullopt.
    // When a group is active it contains information used for synchronizing
    // with the end of the group kernel.
    std::optional<group_info> m_group_info;

    bool is_group_active() const noexcept { return m_group_info.has_value(); }

  public:
    communicator_impl(context_impl* ctxt)
    : communicator_base(ctxt)
    , m_context(ctxt)
    {
    }

    auto& get_heap() noexcept { return m_context->get_heap(); }

    bool is_stream_aware() const noexcept { return true; }

    void start_group()
    {
        if (is_group_active())
        {
            throw std::logic_error(
                "OOMPH Error: start_group called while a group is already active");
        }

        OOMPH_CHECK_NCCL_RESULT(ncclGroupStart());
        m_group_info.emplace();
    }

    void end_group()
    {
        if (!is_group_active())
        {
            throw std::logic_error(
                "OOMPH Error: end_group called without an active group");
        }

        OOMPH_CHECK_NCCL_RESULT(ncclGroupEnd());

        if (m_group_info->has_operations())
        {
            // All streams used in a NCCL group synchronize with the end of the group.
            // We arbitrarily pick the last stream to synchronize against.
            m_group_info->m_event.record(*m_group_info->m_last_stream);
        }
        m_group_info.reset();
    }

    nccl_request send(context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst,
        [[maybe_unused]] tag_type tag, void* stream)
    {
        if (dst == m_context->rank() && !is_group_active())
        {
            throw std::runtime_error(
                "oomph NCCL backend: self-send/recv requires an active NCCL group. "
                "Use start_group()/end_group() around self-send/recv operations.");
        }
        const_device_guard dg(ptr);
        OOMPH_CHECK_NCCL_RESULT(ncclSend(dg.data(), size, ncclChar, dst, m_context->get_comm(),
            static_cast<cudaStream_t>(stream)));

        if (is_group_active())
        {
            m_group_info->m_last_stream = static_cast<cudaStream_t>(stream);
            // The event is stored now, but recorded only in end_group. Until
            // an event has been recorded the event is never ready.
            return {m_group_info->m_event};
        }
        else
        {
            detail::cached_cuda_event event;
            event.record(static_cast<cudaStream_t>(stream));
            return {std::move(event)};
        }
    }

    nccl_request recv(context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src,
        [[maybe_unused]] tag_type tag, void* stream)
    {
        if (src == m_context->rank() && !is_group_active())
        {
            throw std::runtime_error(
                "oomph NCCL backend: self-send/recv requires an active NCCL group. "
                "Use start_group()/end_group() around self-send/recv operations.");
        }
        device_guard dg(ptr);
        OOMPH_CHECK_NCCL_RESULT(ncclRecv(dg.data(), size, ncclChar, src, m_context->get_comm(),
            static_cast<cudaStream_t>(stream)));

        if (is_group_active())
        {
            m_group_info->m_last_stream = static_cast<cudaStream_t>(stream);
            // The event is stored now, but recorded only in end_group. Until
            // an event has been recorded the event is never ready.
            return {m_group_info->m_event};
        }
        else
        {
            detail::cached_cuda_event event;
            event.record(static_cast<cudaStream_t>(stream));
            return {std::move(event)};
        }
    }

    send_request send(context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst,
        tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb, std::size_t* scheduled,
        void* stream)
    {
        auto req = send(ptr, size, dst, tag, stream);
        auto s = m_req_state_factory.make(m_context, this, scheduled, dst, tag, std::move(cb),
            std::move(req));
        s->create_self_ref();
        m_send_reqs.enqueue(s.get());
        return {std::move(s)};
    }

    recv_request recv(context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src,
        tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb, std::size_t* scheduled,
        void* stream)
    {
        auto req = recv(ptr, size, src, tag, stream);
        auto s = m_req_state_factory.make(m_context, this, scheduled, src, tag, std::move(cb),
            std::move(req));
        s->create_self_ref();
        m_recv_reqs.enqueue(s.get());
        return {std::move(s)};
    }

    shared_recv_request shared_recv(context_impl::heap_type::pointer& ptr, std::size_t size,
        rank_type src, tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb,
        std::atomic<std::size_t>* scheduled, void* stream)
    {
        auto req = recv(ptr, size, src, tag, stream);
        auto s = std::make_shared<detail::shared_request_state>(m_context, this, scheduled, src,
            tag, std::move(cb), std::move(req));
        s->create_self_ref();
        m_context->m_req_queue.enqueue(s.get());
        return {std::move(s)};
    }

    void progress()
    {
        if (is_group_active())
        {
            // If we're inside an active group no work has been submitted yet.
            // We cannot make progress so we disallow calling progress. wait
            // also calls progress and would deadlock if called within an active
            // group.
            throw std::logic_error(
                "OOMPH Error: Calling progress while a NCCL group is active is not allowed");
        }

        // Communication progresses independently, but requests must be marked
        // ready and callbacks must be invoked.
        m_send_reqs.progress();
        m_recv_reqs.progress();
        m_context->progress();
    }

    bool cancel_recv(detail::request_state*) { return false; }
};

} // namespace oomph
