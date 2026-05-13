set(target_name
  pict_benchmark
)

set(pict_benchmark_output
  pict-benchmark
)

set(pict_benchmark_src
  ${CMAKE_CURRENT_SOURCE_DIR}/pict_benchmark.cpp
)

add_executable(${target_name}
  ${pict_benchmark_src}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
  FILES
    ${pict_benchmark_src}
)

target_link_libraries(${target_name}
  PRIVATE
    PictBuildOptions
    pict_api
)

set_target_properties(${target_name}
  PROPERTIES
    FOLDER tools
    OUTPUT_NAME ${pict_benchmark_output}
)

install(
  PROGRAMS $<TARGET_FILE:${target_name}>
  TYPE BIN
)
