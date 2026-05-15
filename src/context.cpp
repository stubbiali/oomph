/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <hwmalloc/numa.hpp>
#include <hwmalloc/heap_config.hpp>
#include <oomph/config.hpp>

// paths relative to backend
#include <context.hpp>
#include <communicator.hpp>
#include <../message_buffer.hpp>
#include <../communicator_set.hpp>
#include <../util/heap_pimpl_src.hpp>

OOMPH_INSTANTIATE_HEAP_PIMPL(oomph::context_impl)
OOMPH_INSTANTIATE_HEAP_PIMPL(oomph::detail::message_buffer::heap_ptr_impl)

namespace oomph
{

context::context(MPI_Comm comm, bool thread_safe, //unsigned int num_tag_ranges,
    hwmalloc::heap_config const& heap_config)
: m_mpi_comm{comm}
, m(m_mpi_comm.get(), thread_safe, heap_config)
, m_schedule{std::make_unique<schedule>()}
//, m_tag_range_factory(num_tag_ranges, m->num_tag_bits())
{
}

context::~context() { communicator_set::get().erase(m.get()); }

communicator
context::get_communicator()//unsigned int tr)
{
    return {m->get_communicator(), &(m_schedule->scheduled_recvs)};
    //, m_tag_range_factory.create(tr),
    //    m_tag_range_factory.create(tr, true)};
}

rank_type
context::rank() const noexcept
{
    return m->rank();
}

rank_type
context::size() const noexcept
{
    return m->size();
}

rank_type
context::local_rank() const noexcept
{
    return m->topology().local_rank();
}

rank_type
context::local_size() const noexcept
{
    return m->topology().local_size();
}

const char*
context::get_transport_option(const std::string& opt)
{
    return m->get_transport_option(opt);
}

detail::message_buffer
context::make_buffer_core(std::size_t size)
{
    return m->get_heap().allocate(size, hwmalloc::numa().local_node());
}

detail::message_buffer
context::make_buffer_core(void* ptr, std::size_t size)
{
    return m->get_heap().register_user_allocation(ptr, size);
}

#if OOMPH_ENABLE_DEVICE
detail::message_buffer
context::make_buffer_core(std::size_t size, int id)
{
    return m->get_heap().allocate(size, hwmalloc::numa().local_node(), id);
}

detail::message_buffer
context::make_buffer_core(void* device_ptr, std::size_t size, int device_id)
{
    return m->get_heap().register_user_allocation(device_ptr, device_id, size);
}

detail::message_buffer
context::make_buffer_core(void* ptr, void* device_ptr, std::size_t size, int device_id)
{
    return m->get_heap().register_user_allocation(ptr, device_ptr, device_id, size);
}
#endif

} // namespace oomph
