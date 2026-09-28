# ---------------------------------------------------------------------
# global implementation detail options
# ---------------------------------------------------------------------
set(OOMPH_USE_FAST_PIMPL OFF CACHE BOOL "store private implementations on stack")
set(OOMPH_ENABLE_BARRIER ON CACHE BOOL "enable thread barrier (disable for task based runtime)")
set(OOMPH_RECURSION_DEPTH "20" CACHE STRING "Callback recursion depth")
mark_as_advanced(OOMPH_USE_FAST_PIMPL)

# ---------------------------------------------------------------------
# compiler and linker flags
# ---------------------------------------------------------------------
set(cxx_lang "$<COMPILE_LANGUAGE:CXX>")
set(cxx_lang_gnu "$<COMPILE_LANG_AND_ID:CXX,GNU>")
set(cxx_lang_clang "$<COMPILE_LANG_AND_ID:CXX,Clang>")
set(cxx_lang_intel "$<COMPILE_LANG_AND_ID:CXX,Intel>")
#set(cuda_lang "$<COMPILE_LANGUAGE:CUDA>")
set(fortran_lang "$<COMPILE_LANGUAGE:Fortran>")
set(fortran_lang_gnu "$<COMPILE_LANG_AND_ID:Fortran,GNU>")
set(fortran_lang_intel "$<COMPILE_LANG_AND_ID:Fortran,Intel>")
set(fortran_lang_cray "$<COMPILE_LANG_AND_ID:Fortran,Cray>")

function(oomph_target_compile_options target)
    set_target_properties(${target} PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE ON)
    target_compile_options(${target} PRIVATE
        # flags for CXX builds
        $<${cxx_lang}:$<BUILD_INTERFACE:-Wall -Wextra>>
        $<${cxx_lang_gnu}:$<BUILD_INTERFACE:-Wpedantic -Wno-unknown-pragmas 
            -Wno-unused-local-typedefs>>
        $<${cxx_lang_clang}:$<BUILD_INTERFACE:-Wno-c++17-extensions -Wno-unused-lambda-capture>>
        $<${cxx_lang_intel}:$<BUILD_INTERFACE:-Wno-unknown-pragmas>>
        # flags for CUDA builds
        #$<${cuda_lang}:$<BUILD_INTERFACE:-Xcompiler=-Wall -Wextra -Wno-unknown-pragmas --default-stream per-thread>>
        # flags for Fortran builds
        $<${fortran_lang_gnu}:$<BUILD_INTERFACE:-cpp -ffree-line-length-none>>
        $<${fortran_lang_intel}:$<BUILD_INTERFACE:-cpp>>
        $<${fortran_lang_cray}:$<BUILD_INTERFACE:-eZ>>
    )
endfunction()

function(oomph_target_link_options target)
    target_link_libraries(${target} PUBLIC oomph)
    target_link_libraries(${target} PUBLIC oomph_common)
endfunction()

function(oomph_shared_lib_options target)
    oomph_target_compile_options(${target})
    oomph_target_link_options(${target})
    target_link_libraries(${target} PUBLIC HWMALLOC::hwmalloc)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT APPLE)
        target_link_libraries(${target} PUBLIC atomic)
    endif()
endfunction()

# ---------------------------------------------------------------------
# common library (static): independent of backend
# ---------------------------------------------------------------------
add_library(oomph_common STATIC)
oomph_target_compile_options(oomph_common)
target_link_libraries(oomph_common PUBLIC oomph)

# ---------------------------------------------------------------------
# install rules
# ---------------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS oomph oomph_common
    EXPORT oomph-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
