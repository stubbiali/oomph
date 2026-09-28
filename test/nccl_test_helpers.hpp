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

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

#include <oomph/context.hpp>

namespace oomph::test
{
inline bool
is_nccl_backend(oomph::context const& ctxt)
{
    return ctxt.get_transport_option("name") == std::string("nccl");
}

inline bool
is_nccl_backend()
{
    static const bool result = is_nccl_backend(oomph::context(MPI_COMM_WORLD, false));
    return result;
}

inline void
handle_nccl_thread_safe_exception(std::runtime_error const& e)
{
    if (is_nccl_backend())
    {
        EXPECT_EQ(e.what(), std::string("NCCL not supported with thread_safe = true"));
    }
    else { throw; }
}
} // namespace oomph::test
