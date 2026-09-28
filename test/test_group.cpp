/*
 * ghex-org
 *
 * Copyright (c) 2014-2026, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <oomph/context.hpp>
#include <gtest/gtest.h>
#include "./mpi_runner/mpi_test_fixture.hpp"

TEST_F(mpi_test_fixture, group_progress_wait)
{
    using namespace oomph;
    auto ctxt = context(MPI_COMM_WORLD, false);
    auto comm = ctxt.get_communicator();

    int rank = comm.rank();
    int size = comm.size();
    int next_rank = (rank + 1) % size;
    int prev_rank = (rank + size - 1) % size;

    auto buf_send = comm.make_buffer<int>(1);
    auto buf_recv = comm.make_buffer<int>(1);
    buf_send[0] = rank;

    comm.start_group();
    auto req_send = comm.send(buf_send, next_rank, 0);
    auto req_recv = comm.recv(buf_recv, prev_rank, 0);

    if (ctxt.get_transport_option("name") == std::string("nccl"))
    {
        EXPECT_THROW(comm.progress(), std::logic_error);
        EXPECT_THROW(req_send.wait(), std::logic_error);
        EXPECT_THROW(req_recv.wait(), std::logic_error);
    }
    else
    {
        EXPECT_NO_THROW(comm.progress());
        EXPECT_NO_THROW(req_send.wait());
        EXPECT_NO_THROW(req_recv.wait());
    }
    comm.end_group();

    // For nccl, we threw during wait, so requests are not finished.
    if (ctxt.get_transport_option("name") == std::string("nccl"))
    {
        req_send.wait();
        req_recv.wait();
    }
}
