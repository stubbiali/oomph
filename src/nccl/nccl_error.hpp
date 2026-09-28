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

#include <string>
#include <stdexcept>
#include <iostream>

#include <nccl.h>

#define OOMPH_CHECK_NCCL_RESULT(x)                                                                 \
    {                                                                                              \
        ncclResult_t r = x;                                                                        \
        if (r != ncclSuccess && r != ncclInProgress)                                               \
            throw std::runtime_error("OOMPH Error: NCCL Call failed " + std::string(#x) + " = " +  \
                                     std::to_string(r) + " (\"" + ncclGetErrorString(r) +          \
                                     "\") in " + std::string(__FILE__) + ":" +                     \
                                     std::to_string(__LINE__));                                    \
    }
#define OOMPH_CHECK_NCCL_RESULT_NO_THROW(x)                                                        \
    try                                                                                            \
    {                                                                                              \
        OOMPH_CHECK_NCCL_RESULT(x)                                                                 \
    }                                                                                              \
    catch (const std::exception& e)                                                                \
    {                                                                                              \
        std::cerr << e.what() << std::endl;                                                        \
        std::terminate();                                                                          \
    }
