# set all NCCL related options and values

#------------------------------------------------------------------------------
# Enable NCCL support
#------------------------------------------------------------------------------
set(OOMPH_WITH_NCCL OFF CACHE BOOL "Build with NCCL backend")

if (OOMPH_WITH_NCCL)
    find_package(CUDAToolkit REQUIRED)
    find_package(NCCL REQUIRED)
    add_library(oomph_nccl SHARED)
    add_library(oomph::nccl ALIAS oomph_nccl)
    oomph_shared_lib_options(oomph_nccl)
    target_link_libraries(oomph_nccl PUBLIC NCCL::nccl CUDA::cudart)
    install(TARGETS oomph_nccl
        EXPORT oomph-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
