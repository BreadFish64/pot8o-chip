# CMake 3.8 required for 17 to be a valid value for CXX_STANDARD
cmake_minimum_required(VERSION 3.8)

project(pot8o-chip)

# Sanity check : Check that all submodules are present
# =======================================================================

function(check_submodules_present)
    file(READ "${CMAKE_SOURCE_DIR}/.gitmodules" gitmodules)
    string(REGEX MATCHALL "path *= *[^ \t\r\n]*" gitmodules ${gitmodules})
    foreach(module ${gitmodules})
        string(REGEX REPLACE "path *= *" "" module ${module})
        if (NOT EXISTS "${CMAKE_SOURCE_DIR}/${module}/.git")
            message(SEND_ERROR "Git submodule ${module} not found."
                    "Please run: git submodule update --init --recursive")
        endif()
    endforeach()
endfunction()
check_submodules_present()

# Detect current compilation architecture and create standard definitions
# =======================================================================

include(CheckSymbolExists)
function(detect_architecture symbol arch)
    if (NOT DEFINED ARCHITECTURE)
        set(CMAKE_REQUIRED_QUIET 1)
        check_symbol_exists("${symbol}" "" ARCHITECTURE_${arch})
        unset(CMAKE_REQUIRED_QUIET)

        # The output variable needs to be unique across invocations otherwise
        # CMake's crazy scope rules will keep it defined
        if (ARCHITECTURE_${arch})
            set(ARCHITECTURE "${arch}" PARENT_SCOPE)
            set(ARCHITECTURE_${arch} 1 PARENT_SCOPE)
            add_definitions(-DARCHITECTURE_${arch}=1)
        endif()
    endif()
endfunction()

if (NOT ENABLE_GENERIC)
    if (MSVC)
        detect_architecture("_M_AMD64" x86_64)
    else()
        detect_architecture("__x86_64__" x86_64)
    endif()
endif()
if (NOT DEFINED ARCHITECTURE)
    set(ARCHITECTURE "GENERIC")
    set(ARCHITECTURE_GENERIC 1)
    add_definitions(-DARCHITECTURE_GENERIC=1)
endif()
message(STATUS "Target architecture: ${ARCHITECTURE}")


# Configure compilation flags
# ===========================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

    if (MINGW)
        add_definitions(-DMINGW_HAS_SECURE_API)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf")

        if (MINGW_STATIC_BUILD)
            add_definitions(-DQT_STATICPLUGIN)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
        endif()
    endif()
else()
    # Silence "deprecation" warnings
    add_definitions(/D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_DEPRECATE /D_SCL_SECURE_NO_WARNINGS)
    # Avoid windows.h junk
    add_definitions(/DNOMINMAX)
    # Avoid windows.h from including some usually unused libs like winsocks.h, since this might cause some redefinition errors.
    add_definitions(/DWIN32_LEAN_AND_MEAN)

    # set up output paths for executable binaries (.exe-files, and .dll-files on DLL-capable platforms)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING "" FORCE)

    # Tweak optimization settings
    # As far as I can tell, there's no way to override the CMake defaults while leaving user
    # changes intact, so we'll just clobber everything and say sorry.
    message(STATUS "Cache compiler flags ignored, please edit CMakeLists.txt to change the flags.")

    # /W3 - Level 3 warnings
    # /MP - Multi-threaded compilation
    # /Zi - Output debugging information
    # /Zo - enhanced debug info for optimized builds
    # /permissive- - enables stricter C++ standards conformance checks
    set(CMAKE_C_FLAGS   "/W3 /MP /Zi /Zo /permissive-" CACHE STRING "" FORCE)
    # /EHsc - C++-only exception handling semantics
    # /std:c++latest - Latest available C++ standard
    # /Zc:throwingNew - let codegen assume `operator new` will never return null
    # /Zc:inline - let codegen omit inline functions in object files
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} /EHsc /std:c++latest /Zc:throwingNew,inline" CACHE STRING "" FORCE)

    # /MDd - Multi-threaded Debug Runtime DLL
    set(CMAKE_C_FLAGS_DEBUG   "/Od /MDd" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" CACHE STRING "" FORCE)

    # /Ox - Optimization level 3
    # /GS- - No stack buffer overflow checks
    # /MD - Multi-threaded runtime DLL
    set(CMAKE_C_FLAGS_RELEASE   "/Ox /GS- /MD" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}" CACHE STRING "" FORCE)

    set(CMAKE_EXE_LINKER_FLAGS_DEBUG   "/DEBUG /MANIFEST:NO" CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/DEBUG /MANIFEST:NO /INCREMENTAL:NO /OPT:REF,ICF" CACHE STRING "" FORCE)
endif()

# Set file offset size to 64 bits.
#
# On modern Unixes, this is typically already the case. The lone exception is
# glibc, which may default to 32 bits. glibc allows this to be configured
# by setting _FILE_OFFSET_BITS.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(-D_FILE_OFFSET_BITS=64)
endif()

#add_definitions(-DSINGLETHREADED)
# CMake seems to only define _DEBUG on Windows
set_property(DIRECTORY APPEND PROPERTY
    COMPILE_DEFINITIONS $<$<CONFIG:Debug>:_DEBUG> $<$<NOT:$<CONFIG:Debug>>:NDEBUG>)

set(SDL2_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/SDL2/include" CACHE PATH "Path to SDL2 headers")
set(SDL2_LIBRARY "${CMAKE_SOURCE_DIR}/SDL2/lib/x64/SDL2.lib" CACHE PATH "Path to SDL2 library")
set(SDL2_DLL_DIR "${CMAKE_SOURCE_DIR}/SDL2/lib/x64/" CACHE PATH "Path to SDL2.dll")

add_library(SDL2 INTERFACE)
target_link_libraries(SDL2 INTERFACE "${SDL2_LIBRARY}")
target_include_directories(SDL2 INTERFACE "${SDL2_INCLUDE_DIR}")

include_directories("${CMAKE_SOURCE_DIR}/xbyak/xbyak")

# Include source code
# ===================

# This function should be passed a list of all files in a target. It will automatically generate
# file groups following the directory hierarchy, so that the layout of the files in IDEs matches the
# one in the filesystem.
function(create_target_directory_groups target_name)
    # Place any files that aren't in the source list in a separate group so that they don't get in
    # the way.
    source_group("Other Files" REGULAR_EXPRESSION ".")

    get_target_property(target_sources "${target_name}" SOURCES)

    foreach(file_name IN LISTS target_sources)
        get_filename_component(dir_name "${file_name}" PATH)
        # Group names use '\' as a separator even though the entire rest of CMake uses '/'...
        string(REPLACE "/" "\\" group_name "${dir_name}")
        source_group("${group_name}" FILES "${file_name}")
    endforeach()
endfunction()

add_subdirectory(pot8o-chip)
add_subdirectory(xbyak)
