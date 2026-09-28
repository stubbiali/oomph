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

#include "cuda_event.hpp"
#include "cuda_event_pool.hpp"

namespace oomph::detail
{
// A cuda_event backed by a cuda_event_pool.
//
// Same semantics as cuda_event, but the event is retrieved from a static
// cuda_event_pool on construction and returned to the pool on destruction.
struct cached_cuda_event
{
    cuda_event m_event;

    cached_cuda_event()
    : m_event(get_cuda_event_pool().pop())
    {
    }
    cached_cuda_event(cached_cuda_event&& other) noexcept = default;
    cached_cuda_event& operator=(cached_cuda_event&& other) noexcept = default;
    cached_cuda_event(const cached_cuda_event&) = delete;
    cached_cuda_event& operator=(const cached_cuda_event&) = delete;
    ~cached_cuda_event() noexcept
    {
        if (m_event) { get_cuda_event_pool().push(std::move(m_event)); }
    }

    operator bool() noexcept { return bool(m_event); }

    void record(cudaStream_t stream) { return m_event.record(stream); }

    bool is_ready() const { return m_event.is_ready(); }

    cudaEvent_t get() { return m_event.get(); }
};
} // namespace oomph::detail
