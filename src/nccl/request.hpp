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

#include <variant>

#include <cuda_runtime.h>

#include "cuda_error.hpp"
#include "cuda_event.hpp"
#include "group_cuda_event.hpp"

namespace oomph
{
struct nccl_request
{
    bool is_ready() const
    {
        return std::visit([](auto const& event) { return event.is_ready(); }, m_event);
    }

    // We store either a single event for a particular request, or a shared
    // event that signals the end of a NCCL group.
    std::variant<detail::cached_cuda_event, detail::group_cuda_event> m_event;
};
} // namespace oomph
