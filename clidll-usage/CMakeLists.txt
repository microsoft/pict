set(target_name
  pict_dll_usage
)

set(pict_dll_usage_output
  pictclidllusage
)

set(pict_dll_usage_src
  ${CMAKE_CURRENT_SOURCE_DIR}/pictclidll-sample.cpp
)

add_executable(${target_name}
  ${pict_dll_usage_src}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
  FILES
    ${pict_dll_usage_src}
)

target_link_libraries(${target_name}
  PRIVATE
    PictBuildOptions
    pict_dll
)

set_target_properties(${target_name}
  PROPERTIES
    FOLDER clidll-usage
    OUTPUT_NAME ${pict_dll_usage_output}
)
