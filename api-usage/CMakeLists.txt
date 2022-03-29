set(target_name
  pict_api_usage
)

set(pict_api_usage_output
  pictapisamples
)

set(pict_api_usage_src
  ${CMAKE_CURRENT_SOURCE_DIR}/pictapi-sample.cpp
)

add_executable(${target_name}
  ${pict_api_usage_src}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
  FILES
    ${pict_api_usage_src}
)

target_link_libraries(${target_name}
  PRIVATE
    PictBuildOptions
    pict_api
)

set_target_properties(${target_name}
  PROPERTIES
    FOLDER api-usage
    OUTPUT_NAME ${pict_api_usage_output}
)
