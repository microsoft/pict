include(GNUInstallDirs)

set(target_name
  pict_cli
)

set(pict_cli_output
  pict
)

set(pict_cli_inc
  ${CMAKE_CURRENT_SOURCE_DIR}/ccommon.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cmdline.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cparser.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ctokenizer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gcd.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gcdexcl.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gcdmodel.h
  ${CMAKE_CURRENT_SOURCE_DIR}/model.h
  ${CMAKE_CURRENT_SOURCE_DIR}/resource.h
  ${CMAKE_CURRENT_SOURCE_DIR}/strings.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ver.h
)

set(pict_cli_src
  ${CMAKE_CURRENT_SOURCE_DIR}/ccommon.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cmdline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/common.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cparser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ctokenizer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gcd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gcdexcl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gcdmodel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/model.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mparser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pict.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/strings.cpp
)

set(pict_cli_dll_src
  ${pict_cli_src} PARENT_SCOPE
)

set(pict_cli_rc
  $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/Resource.rc>
)

add_executable(${target_name}
  ${pict_cli_inc}
  ${pict_cli_src}
  ${pict_cli_rc}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
  FILES
    ${pict_cli_inc}
    ${pict_cli_src}
)

target_link_libraries(${target_name}
  PRIVATE
    PictBuildOptions
    pict_api
)

set_target_properties(${target_name}
  PROPERTIES
    FOLDER src
    OUTPUT_NAME ${pict_cli_output}
)

install(
  PROGRAMS $<TARGET_FILE:${target_name}>
  TYPE BIN
)

install(
  FILES
    ${CMAKE_SOURCE_DIR}/doc/pict.md
    ${CMAKE_SOURCE_DIR}/LICENSE.TXT
  TYPE DOC
)
