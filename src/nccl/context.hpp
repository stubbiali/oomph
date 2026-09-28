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

#include <hwmalloc/heap.hpp>
#include <hwmalloc/heap_config.hpp>

#include <oomph/config.hpp>

// paths relative to backend
#include "../context_base.hpp"
#include "nccl_communicator.hpp"
#include "region.hpp"
#include "request_queue.hpp"

namespace oomph
{
class context_impl : public context_base
{
  public:
    using region_type = region;
    using device_region_type = region;
    using heap_type = hwmalloc::heap<context_impl>;

  private:
    heap_type         m_heap;
    detail::nccl_comm m_comm;

  public:
    shared_request_queue m_req_queue;

  private:
    static bool check_not_thread_safe(bool thread_safe)
    {
        if (thread_safe)
            throw std::runtime_error("NCCL not supported with thread_safe = true");
        return thread_safe;
    }

  public:
    context_impl(MPI_Comm comm, bool thread_safe, hwmalloc::heap_config const& heap_config)
    : context_base(comm, check_not_thread_safe(thread_safe))
    , m_heap{this, heap_config}
    , m_comm{oomph::detail::nccl_comm{comm}}
    {
    }

    context_impl(context_impl const&) = delete;
    context_impl(context_impl&&) = delete;

    ncclComm_t get_comm() const noexcept { return m_comm.get(); }

    region make_region(void* ptr) const { return {ptr}; }

    auto& get_heap() noexcept { return m_heap; }

    communicator_impl* get_communicator();

    void progress() { m_req_queue.progress(); }

    bool cancel_recv(detail::shared_request_state*) { return false; }

    const char* get_transport_option(const std::string& opt) const;
};

template<>
inline region
register_memory<context_impl>(context_impl& c, void* ptr, std::size_t)
{
    return c.make_region(ptr);
}

#if OOMPH_ENABLE_DEVICE
template<>
inline region
register_device_memory<context_impl>(context_impl& c, int, void* ptr, std::size_t)
{
    return c.make_region(ptr);
}
#endif

} // namespace oomph
