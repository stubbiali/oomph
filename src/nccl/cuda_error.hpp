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

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include <cuda_runtime.h>

#define OOMPH_CHECK_CUDA_RESULT(x)                                                                 \
    if (x != cudaSuccess)                                                                          \
        throw std::runtime_error("OOMPH Error: CUDA Call failed " + std::string(#x) + " (" +       \
                                 std::string(cudaGetErrorString(x)) + ") in " +                    \
                                 std::string(__FILE__) + ":" + std::to_string(__LINE__));

#define OOMPH_CHECK_CUDA_RESULT_NO_THROW(x)                                                        \
    try                                                                                            \
    {                                                                                              \
        OOMPH_CHECK_CUDA_RESULT(x)                                                                 \
    }                                                                                              \
    catch (const std::exception& e)                                                                \
    {                                                                                              \
        std::cerr << e.what() << std::endl;                                                        \
        std::terminate();                                                                          \
    }
