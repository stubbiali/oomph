include(oomph_git_submodule)
include(oomph_external_project)

if(OOMPH_GIT_SUBMODULE)
    update_git_submodules()
endif()

# ---------------------------------------------------------------------
# MPI setup
# ---------------------------------------------------------------------
find_package(MPI REQUIRED COMPONENTS CXX)

# ---------------------------------------------------------------------
# Boost setup
# ---------------------------------------------------------------------
find_package(Boost REQUIRED)

# ---------------------------------------------------------------------
# hwmalloc setup
# ---------------------------------------------------------------------
cmake_dependent_option(OOMPH_USE_BUNDLED_HWMALLOC "Use bundled hwmalloc lib." ON
    "OOMPH_USE_BUNDLED_LIBS" OFF)
if(OOMPH_USE_BUNDLED_HWMALLOC)
    check_git_submodule(hwmalloc ext/hwmalloc)
    add_subdirectory(ext/hwmalloc)
    add_library(HWMALLOC::hwmalloc ALIAS hwmalloc)
else()
    find_package(HWMALLOC REQUIRED)
endif()

# ---------------------------------------------------------------------
# google test setup
# ---------------------------------------------------------------------
cmake_dependent_option(OOMPH_USE_BUNDLED_GTEST "Use bundled googletest lib." ON
    "OOMPH_USE_BUNDLED_LIBS" OFF)
if (OOMPH_WITH_TESTING)
    if(OOMPH_USE_BUNDLED_GTEST)
        add_external_cmake_project(
            NAME googletest
            PATH ext/googletest
            INTERFACE_NAME ext-gtest
            LIBS libgtest.a libgtest_main.a
            CMAKE_ARGS
                "-DCMAKE_BUILD_TYPE=release"
                "-DBUILD_SHARED_LIBS=OFF"
                "-DBUILD_GMOCK=OFF")
        # on some systems we need link explicitly against threads
        if (TARGET ext-gtest)
            find_package (Threads)
            target_link_libraries(ext-gtest INTERFACE Threads::Threads)
        endif()
    else()
        # Use system provided google test
        find_package(GTest REQUIRED)
        add_library(ext-gtest INTERFACE)
        if (${CMAKE_VERSION} VERSION_LESS "3.20.0")
            target_link_libraries(ext-gtest INTERFACE GTest::GTest GTest::Main)
        else()
            target_link_libraries(ext-gtest INTERFACE GTest::gtest GTest::gtest_main)
        endif()
    endif()
endif()
