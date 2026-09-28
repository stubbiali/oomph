add_library(oomph_private_nccl_headers INTERFACE)
target_include_directories(oomph_private_nccl_headers INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_link_libraries(oomph_nccl PRIVATE oomph_private_nccl_headers)

list(TRANSFORM oomph_sources PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/../
    OUTPUT_VARIABLE oomph_sources_nccl)
target_sources(oomph_nccl PRIVATE ${oomph_sources_nccl})
target_sources(oomph_nccl PRIVATE context.cpp cuda_event_pool.cpp)
