find_package(OpenMP REQUIRED)

# list of tests to be executed
set(parallel_tests_f
    context_f
    message_f
    barrier_f
    send_recv_ft_f
    send_recv_cb_f)
    #send_multi_f
    #send_multi_2_f)

function(reg_parallel_test_f t_ lib n nthr)
    set(t ${t_}_${lib})
    add_executable(${t} test_${t_}.f90)
    set_target_properties(${t} PROPERTIES Fortran_MODULE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${t}_Mod)
    oomph_target_compile_options(${t})
    set_target_properties(${t} PROPERTIES LINKER_LANGUAGE Fortran)
    target_link_libraries(${t} PRIVATE oomph_f)
    target_link_libraries(${t} PRIVATE oomph_common)
    target_link_libraries(${t} PRIVATE oomph_${lib})
    target_link_libraries(${t} PUBLIC MPI::MPI_Fortran)
    target_link_libraries(${t} PRIVATE OpenMP::OpenMP_Fortran)
    target_include_directories(${t} PRIVATE
        $<INSTALL_INTERFACE:..>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../..>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../../..>)
    if("${MPIEXEC_EXECUTABLE}" STREQUAL "")
      add_test(NAME ${t} COMMAND $<TARGET_FILE:${t}>)
    else()
      add_test(
          NAME ${t}
          COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${n} ${MPIEXEC_PREFLAGS}
              $<TARGET_FILE:${t}> ${MPIEXEC_POSTFLAGS})
    endif()
    set_tests_properties(${t} PROPERTIES
        ENVIRONMENT OMP_NUM_THREADS=${nthr}
        LABELS "parallel-ranks-${n}")
endfunction()

if (OOMPH_WITH_MPI)
    foreach(t ${parallel_tests_f})
        reg_parallel_test_f(${t} mpi 4 4)
    endforeach()
endif()

if (OOMPH_WITH_UCX)
    foreach(t ${parallel_tests_f})
        reg_parallel_test_f(${t} ucx 4 4)
    endforeach()
endif()

if (OOMPH_WITH_LIBFABRIC)
    foreach(t ${parallel_tests_f})
        reg_parallel_test_f(${t} libfabric 4 4)
    endforeach()
endif()
