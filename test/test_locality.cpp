/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <oomph/context.hpp>
#include <gtest/gtest.h>
#include "./mpi_runner/mpi_test_fixture.hpp"
#include <vector>
#include <iomanip>
#include <utility>
#include <unistd.h>
#include <limits.h>
#include <cstring>


#ifdef __APPLE__
#define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
#endif

// test locality by collecting all local ranks
TEST_F(mpi_test_fixture, locality_enumerate)
{
    using namespace oomph;
    auto ctxt = context(MPI_COMM_WORLD, false);
    auto comm = ctxt.get_communicator();

    // test self
    EXPECT_TRUE(comm.is_local(comm.rank()));

    // check for symmetry
    auto local_ranks = comm.make_buffer<int>(comm.size());
    // host names must be contained in a message-compatible data
    auto my_host_name = comm.make_buffer<char>(HOST_NAME_MAX + 1);
    for (auto& c : my_host_name) c = 0;
    auto other_host_name = comm.make_buffer<char>(HOST_NAME_MAX + 1);
    for (auto& c : other_host_name) c = 0;
    gethostname(my_host_name.data(), HOST_NAME_MAX + 1);
    for (int r = 0; r < comm.size(); ++r)
    {
        if (r == comm.rank())
        {
            for (int rr = 0; rr < comm.size(); ++rr)
            { local_ranks[rr] = comm.is_local(rr) ? 1 : 0; }
            for (int rr = 0; rr < comm.size(); ++rr)
            {
                if (rr != comm.rank())
                {
                    comm.send(local_ranks, rr, 0).wait();
                    comm.send(my_host_name, rr, 1).wait();
                }
            }
        }
        else
        {
            const int is_neighbor = comm.is_local(r) ? 1 : 0;
            comm.recv(local_ranks, r, 0).wait();
            comm.recv(other_host_name, r, 1).wait();
            EXPECT_EQ(is_neighbor, local_ranks[comm.rank()]);
            if (is_neighbor)
                for (int rr = 0; rr < comm.size(); ++rr)
                { EXPECT_EQ((comm.is_local(rr) ? 1 : 0), local_ranks[rr]); }
            const int equal_hosts =
                (std::strcmp(my_host_name.data(), other_host_name.data()) == 0) ? 1 : 0;
            if (is_neighbor == 1) { EXPECT_EQ(equal_hosts, 1); }
        }
    }
}
