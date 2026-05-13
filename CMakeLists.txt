#########################################################################

# CMAKE MINIMAL VERSION
cmake_minimum_required(VERSION 3.10)

# PROJECT PROPERTIES
project("CMM compiler" LANGUAGES CXX)

# 32-bit host build (-m32) only if your GCC has i686/multilib libs (many Win64 MinGW installs do not).
option(CMM_32BIT_HOST "Compile the compiler itself with -m32" OFF)
if(CMM_32BIT_HOST)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
endif()

#########################################################################

# ASSEMBLY OPTIONS

if(NOT CMAKE_CXX_EXTENSIONS)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

add_compile_options(
    -Wall
    -Wpedantic

    -Wno-multichar
    -Wno-format-contains-nul
    -Wno-sign-compare
)

#########################################################################


# Windows: build sfasmlib_runtime.c -> ${CMAKE_BINARY_DIR}/sfasmlib.dll on each build (when stale).
# CMM_SFASM_GCC: persistent path to mingw32 gcc.exe (i686), stored in CMakeCache.txt — no shell env needed.
# Autodetects C:/msys64/... etc. when the cache entry is empty on first configure.
# Disable DLL build: cmake .. -DCMM_BUILD_SFASM_RUNTIME=OFF
if(WIN32)
    option(CMM_BUILD_SFASM_RUNTIME "Build sfasmlib.dll from sfasmlib_runtime.c into the binary dir" ON)

    set(CMM_SFASM_GCC "" CACHE FILEPATH
        "Path to MSYS2 mingw32 gcc.exe (i686), e.g. C:/msys64/mingw32/bin/gcc.exe. Leave empty to auto-detect.")

    if("${CMM_SFASM_GCC}" STREQUAL "")
        foreach(_mgcc IN ITEMS
                "C:/msys64/mingw32/bin/gcc.exe"
                "D:/msys64/mingw32/bin/gcc.exe"
                "$ENV{ProgramFiles}/msys64/mingw32/bin/gcc.exe"
                "$ENV{LOCALAPPDATA}/Programs/msys64/mingw32/bin/gcc.exe")
            if(EXISTS "${_mgcc}")
                set(CMM_SFASM_GCC "${_mgcc}" CACHE FILEPATH "Path to MSYS2 mingw32 gcc.exe (i686)." FORCE)
                message(STATUS "Auto-selected CMM_SFASM_GCC: ${CMM_SFASM_GCC}")
                break()
            endif()
        endforeach()
    endif()

    if(CMM_BUILD_SFASM_RUNTIME)
        if(CMM_SFASM_GCC AND EXISTS "${CMM_SFASM_GCC}")
            add_custom_command(
                OUTPUT "${CMAKE_BINARY_DIR}/sfasmlib.dll"
                COMMAND "${CMAKE_COMMAND}" -E env "SFASM_GCC32=${CMM_SFASM_GCC}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/sfasmlib/build_runtime.bat"
                        "${CMAKE_BINARY_DIR}"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sfasmlib"
                DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/sfasmlib/sfasmlib_runtime.c"
                        "${CMAKE_CURRENT_SOURCE_DIR}/sfasmlib/build_runtime.bat"
                COMMENT "Building sfasmlib.dll (${CMM_SFASM_GCC})"
                VERBATIM)
        else()
            if(NOT "${CMM_SFASM_GCC}" STREQUAL "")
                message(WARNING "CMM_SFASM_GCC points to missing file: \"${CMM_SFASM_GCC}\" — sfasmlib build may fail.")
            endif()
            add_custom_command(
                OUTPUT "${CMAKE_BINARY_DIR}/sfasmlib.dll"
                COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/sfasmlib/build_runtime.bat"
                        "${CMAKE_BINARY_DIR}"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sfasmlib"
                DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/sfasmlib/sfasmlib_runtime.c"
                        "${CMAKE_CURRENT_SOURCE_DIR}/sfasmlib/build_runtime.bat"
                COMMENT "Building sfasmlib.dll (sfasmlib_runtime.c)"
                VERBATIM)
        endif()
        add_custom_target(cmm_sfasm_runtime ALL DEPENDS "${CMAKE_BINARY_DIR}/sfasmlib.dll")
    endif()
endif()

# TEXT LIBRARY

set(TEXT_LIB_PATH)
set(TEXT_LIB_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libtext)

find_library(TEXT_LIB_PATH NAMES text PATHS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${NCPU_DIR}/libtext)

if (NOT TEXT_LIB_PATH)
    set(TEXT_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/libtext)
    add_subdirectory(${TEXT_LIB_SRC_DIR} ${TEXT_LIB_DIR} ${PARENT_SCOPE})
    set(TEXT_LIB_PATH text)
endif()

# TREE

set(TREE_LIB_NAME "tree")
set(TREE_LIB_PATH ${TREE_LIB_NAME})
set(TREE_LIB_SRC_DIR ${TREE_LIB_NAME})

set(TREE_LIB_SRC
    tree/tree.cpp
)

add_library(${TREE_LIB_NAME} STATIC)
target_include_directories(${TREE_LIB_NAME} PRIVATE ${TREE_LIB_NAME})
target_sources(${TREE_LIB_NAME} PRIVATE ${TREE_LIB_SRC})

# CMM FRONTEND

set(LANG_FRONTEND_NAME "cmm_frontend")

set(LANG_FRONTEND_SRC
    frontend/main.cpp
    frontend/parsing.cpp
    frontend/tree_recording.cpp
)

add_executable(${LANG_FRONTEND_NAME} ${LANG_FRONTEND_SRC})

target_include_directories(${LANG_FRONTEND_NAME} PRIVATE ${TEXT_LIB_SRC_DIR} PRIVATE ${TREE_LIB_SRC_DIR})
target_link_libraries(${LANG_FRONTEND_NAME} ${TEXT_LIB_PATH} ${TREE_LIB_PATH})

# LANG OPTIMIZER

set(LANG_OPTIMIZER_NAME "lang_optimizer")

set(LANG_OPTIMIZER_SRC
    optimizer/main.cpp
    optimizer/derivative.cpp
    optimizer/optimizer.cpp
    optimizer/tree_reading.cpp
    optimizer/tree_recording.cpp
)

add_executable(${LANG_OPTIMIZER_NAME} ${LANG_OPTIMIZER_SRC})

target_include_directories(${LANG_OPTIMIZER_NAME} PRIVATE ${TEXT_LIB_SRC_DIR}  PRIVATE ${TREE_LIB_SRC_DIR})
target_link_libraries(${LANG_OPTIMIZER_NAME} ${TEXT_LIB_PATH} ${TREE_LIB_PATH})

# LANG REVERSED FRONTEND

set(CMM_REV_FRONTEND_NAME "cmm_frontend_rev")

set(CMM_REV_FRONTEND_SRC
    frontend_reversed/main.cpp
    frontend_reversed/frontend_rev.cpp
    frontend_reversed/tree_reading.cpp
)

add_executable(${CMM_REV_FRONTEND_NAME} ${CMM_REV_FRONTEND_SRC})

target_include_directories(${CMM_REV_FRONTEND_NAME} PRIVATE ${TEXT_LIB_SRC_DIR} PRIVATE ${TREE_LIB_SRC_DIR})
target_link_libraries(${CMM_REV_FRONTEND_NAME} ${TEXT_LIB_PATH} ${TREE_LIB_PATH})

# LANG_COMPILE

set(LANG_COMPILE_NAME "lang_compile")

set(LANG_COMPILE_SRC
    backend/main.cpp
    backend/backend.cpp
    backend/exe_creator.cpp
    backend/tree_reading.cpp
)

add_executable(${LANG_COMPILE_NAME} ${LANG_COMPILE_SRC})

target_include_directories(${LANG_COMPILE_NAME} PRIVATE ${TEXT_LIB_SRC_DIR} PRIVATE ${TREE_LIB_SRC_DIR})
target_link_libraries(${LANG_COMPILE_NAME} ${TEXT_LIB_PATH} ${TREE_LIB_PATH})

if(WIN32 AND CMM_BUILD_SFASM_RUNTIME)
    foreach(_cmm_exe IN ITEMS ${LANG_FRONTEND_NAME} ${LANG_OPTIMIZER_NAME} ${CMM_REV_FRONTEND_NAME} ${LANG_COMPILE_NAME})
        add_dependencies(${_cmm_exe} cmm_sfasm_runtime)
    endforeach()
endif()

##########################################################################
