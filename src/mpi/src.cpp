/*
 * ghex-org
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "./context.hpp"
#include "./communicator.hpp"

namespace oomph
{
communicator_impl*
context_impl::get_communicator()
{
    auto comm = new communicator_impl{this};
    m_comms_set.insert(comm);
    return comm;
}

const char *context_impl::get_transport_option(const std::string &opt) {
    if (opt == "name") {
        return "mpi";
    }
    else {
        return "unspecified";
    }
}

} // namespace oomph

#include "../src.cpp"
