cmake_minimum_required(VERSION 3.13)

# Allow CMake 3.13+ to override options when using FetchContent/add_subdirectory.
# option honors normal variables
if (POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
endif()

# INTERFACE_LINK_LIBRARIES defines the link interface.
if (POLICY CMP0022)
  cmake_policy (SET CMP0022 NEW)
endif()

# PICT version, build, and product info
set(PICT_VERSION_MAJOR 3 CACHE STRING "PICT major version")
set(PICT_VERSION_MINOR 7 CACHE STRING "PICT minor version")
set(PICT_VERSION_BUILD 4 CACHE STRING "PICT build version")
set(PICT_VERSION_SHORT "${PICT_VERSION_MAJOR}.${PICT_VERSION_MINOR}")
set(PICT_VERSION_FULL  "${PICT_VERSION_SHORT}.${PICT_VERSION_BUILD}")

message(STATUS "PICT version ${PICT_VERSION_FULL}")

# Set project name and properties
project(
  PICT
  DESCRIPTION "Pairwise Independent Combinatorial Tool"
  HOMEPAGE_URL "https://github.com/microsoft/pict"
  LANGUAGES CXX
  VERSION ${PICT_VERSION_FULL}
)

# Build configuration
option(PICT_RUN_TESTS_ENABLED        "Enable running tests after build" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(PICT_BUILD_OPTIONS PictBuildOptions)
add_library(${PICT_BUILD_OPTIONS}
  INTERFACE
)

if (MSVC)
  # Enable multi-threaded build with MSVC
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  target_compile_options(${PICT_BUILD_OPTIONS}
    INTERFACE
      /W4 /WX
      /wd4189 # DOUT(arg) has unused parameters. Suppressing the warning here.
  )

  target_compile_definitions(${PICT_BUILD_OPTIONS}
    INTERFACE
      UNICODE
      WIN32_LEAN_AND_MEAN
  )
else()
  target_compile_options(${PICT_BUILD_OPTIONS}
    INTERFACE
      -fPIC # Position-independent code: necessary for static libraries that are linked in dynamic libraries
      -pipe
      -fno-omit-frame-pointer
      -fvisibility=hidden # By default, hide symbols on ELF binaries
      -g # add debug symbols for build pdb
      -pedantic
      -Wall
      -Werror
      $<$<CXX_COMPILER_ID:GNU>:-Wno-unused-but-set-variable>
      -Wno-unused-variable
  )

  # AppleClang is not accepting -flto as linker option
  if(NOT APPLE)
    target_link_options(${PICT_BUILD_OPTIONS}
      INTERFACE
        $<IF:$<CONFIG:Debug>,,LINKER:-flto$<COMMA>-flto-partition=none> #Enable LTO
        $<IF:$<CONFIG:Debug>,,LINKER:--no-undefined> # Have Linker error out if any symbols are undefined.
    )
  endif()
endif()

target_compile_definitions(${PICT_BUILD_OPTIONS}
  INTERFACE
    $<IF:$<CONFIG:Debug>,_DEBUG,NDEBUG>
)

add_subdirectory (api)
add_subdirectory (cli)
add_subdirectory (clidll)

if(PICT_RUN_TESTS_ENABLED)
  enable_testing()
  add_subdirectory(test)
endif()

if(WIN32)
  add_subdirectory(api-usage)
  add_subdirectory(clidll-usage)
endif()

if(WIN32)
  set(CPACK_GENERATOR ZIP NUGET)
else()
  set(CPACK_GENERATOR TGZ)
endif()

set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
  \\.git/
  build/
  ".*~$"
)
set(CPACK_VERBATIM_VARIABLES YES)

execute_process(
  COMMAND git log --pretty=format:%H -n 1
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND git log --pretty=format:%h -n 1
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_SHORT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CPACK_SOURCE_PACKAGE_FILE_NAME
  ${CMAKE_PROJECT_NAME}-${GIT_SHORT_COMMIT_HASH}
)

include(CPack)
