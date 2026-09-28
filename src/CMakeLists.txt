
add_subdirectory(common)

set(oomph_sources
    message_buffer.cpp
    communicator.cpp
    context.cpp
    request.cpp
    communicator_set.cpp
    communicator_state.cpp
    barrier.cpp
)

if (OOMPH_WITH_MPI)
    add_subdirectory(mpi)
endif()

if (OOMPH_WITH_UCX)
    add_subdirectory(ucx)
endif()

if (OOMPH_WITH_LIBFABRIC)
    add_subdirectory(libfabric)
endif()

if (OOMPH_WITH_NCCL)
    add_subdirectory(nccl)
endif()
