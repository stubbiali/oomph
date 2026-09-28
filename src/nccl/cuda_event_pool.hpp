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

#include <cassert>
#include <cstddef>
#include <vector>

#include <cuda_runtime.h>

#include <oomph/util/moved_bit.hpp>

#include "cuda_error.hpp"
#include "cuda_event.hpp"

namespace oomph::detail
{
// Pool of cuda_events.
//
// Simple wrapper over a vector of cuda_events. Events can be popped from the
// pool. New events are created if the pool is empty. Events can be returned to
// the pool for reuse. Events do not need to originate from the pool. Not
// thread-safe.
class cuda_event_pool
{
  private:
    std::vector<cuda_event> m_events;

  public:
    cuda_event_pool(std::size_t expected_pool_size)
    : m_events(expected_pool_size)
    {
    }

    cuda_event_pool(const cuda_event_pool&) = delete;
    cuda_event_pool& operator=(const cuda_event_pool&) = delete;
    cuda_event_pool(cuda_event_pool&& other) noexcept = delete;
    cuda_event_pool& operator=(cuda_event_pool&&) noexcept = delete;

  public:
    cuda_event pop()
    {
        if (m_events.empty()) { return {}; }
        else
        {
            auto event{std::move(m_events.back())};
            m_events.pop_back();
            return event;
        }
    }

    void push(cuda_event&& event) { m_events.push_back(std::move(event)); }
    void clear() { m_events.clear(); }
};

// Get a static instance of a cuda_event_pool.
cuda_event_pool& get_cuda_event_pool();
} // namespace oomph::detail
