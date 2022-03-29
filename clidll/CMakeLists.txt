set(target_name
  pict_dll
)

set(pict_dll_output
  pict
)

set(pict_dll_inc
  ${CMAKE_CURRENT_SOURCE_DIR}/resource.h
)

set(pict_dll_src
  ${CMAKE_CURRENT_SOURCE_DIR}/dllmain.cpp
  $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/Resource.rc>
  $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/pictclidll.def>
)

add_library(${target_name}
  SHARED
    ${pict_dll_inc}
    ${pict_dll_src}
    ${pict_cli_dll_src}
)

add_compile_definitions(${target_name}
  PRIVATE
    PICTCLIDLL_EXPORTS
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
  FILES
    ${pict_dll_inc}
    ${pict_dll_src}
)

target_link_libraries(${target_name}
  PRIVATE
    PictBuildOptions
    pict_api
)

set_target_properties(${target_name}
  PROPERTIES
    FOLDER src
    OUTPUT_NAME ${pict_dll_output}
)
