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

#include <cuda_runtime.h>

#include <oomph/util/moved_bit.hpp>

#include "cuda_error.hpp"

namespace oomph::detail
{
// RAII wrapper for a cudaEvent_t.
//
// Move-only wrapper around cudaEvent_t that automatically destroys the
// underlying event on destruction. Can be used to record events on streams.
struct cuda_event
{
    cudaEvent_t            m_event;
    oomph::util::moved_bit m_moved;
    bool                   m_recorded{false};

    cuda_event()
    {
        OOMPH_CHECK_CUDA_RESULT(cudaEventCreateWithFlags(&m_event, cudaEventDisableTiming));
    }
    cuda_event(cuda_event&& other) noexcept = default;
    cuda_event& operator=(cuda_event&& other) noexcept = default;
    cuda_event(const cuda_event&) = delete;
    cuda_event& operator=(const cuda_event&) = delete;
    ~cuda_event() noexcept
    {
        if (!m_moved) { OOMPH_CHECK_CUDA_RESULT_NO_THROW(cudaEventDestroy(m_event)); }
    }

    operator bool() noexcept { return !m_moved; }

    void record(cudaStream_t stream)
    {
        assert(!m_moved);
        OOMPH_CHECK_CUDA_RESULT(cudaEventRecord(m_event, stream));
        m_recorded = true;
    }

    bool is_ready() const
    {
        if (m_moved || !m_recorded) { return false; }

        cudaError_t res = cudaEventQuery(m_event);
        if (res == cudaSuccess) { return true; }
        else if (res == cudaErrorNotReady) { return false; }
        else
        {
            OOMPH_CHECK_CUDA_RESULT(res);
            return false;
        }
    }

    cudaEvent_t get()
    {
        assert(!m_moved);
        return m_event;
    }
};
} // namespace oomph::detail
