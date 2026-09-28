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

#include <memory>

#include "cached_cuda_event.hpp"

namespace oomph::detail
{
// A shared cuda_event suitable for use with NCCL groups.
//
// A cached_cuda_event stored in a shared_ptr for shared usage between multiple
// requests.
struct group_cuda_event
{
    std::shared_ptr<cached_cuda_event> m_event;

    group_cuda_event()
    : m_event(std::make_shared<cached_cuda_event>())
    {
    }
    group_cuda_event(const group_cuda_event&) = default;
    group_cuda_event& operator=(const group_cuda_event&) = default;
    group_cuda_event(group_cuda_event&&) = default;
    group_cuda_event& operator=(group_cuda_event&&) = default;

    void record(cudaStream_t stream) { m_event->record(stream); }

    bool is_ready() const { return m_event->is_ready(); }

    cudaEvent_t get() { return m_event->get(); }
};
} // namespace oomph::detail
