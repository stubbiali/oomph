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

#include <utility>

// paths relative to backend
#include "handle.hpp"

namespace oomph
{
class region
{
  public:
    using handle_type = handle;

  private:
    void* m_ptr;

  public:
    region(void* ptr)
    : m_ptr{ptr}
    {
    }

    region(region const&) = delete;

    region(region&& r) noexcept
    : m_ptr{std::exchange(r.m_ptr, nullptr)}
    {
    }

    // get a handle to some portion of the region
    handle_type get_handle(std::size_t offset, std::size_t size)
    {
        return {(void*)((char*)m_ptr + offset), size};
    }
};
} // namespace oomph
