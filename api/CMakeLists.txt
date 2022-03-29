set(target_name
  pict_api
)

set(pict_api_output
  pict
)

set(pict_api_inc
  ${CMAKE_CURRENT_SOURCE_DIR}/deriver.h
  ${CMAKE_CURRENT_SOURCE_DIR}/generator.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pictapi.h
  ${CMAKE_CURRENT_SOURCE_DIR}/resource.h
  ${CMAKE_CURRENT_SOURCE_DIR}/trie.h
)

set(pict_api_src
  ${CMAKE_CURRENT_SOURCE_DIR}/combination.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/deriver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/exclusion.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/model.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/parameter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pictapi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/task.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/worklist.cpp
)

add_library(${target_name}
  STATIC
    ${pict_api_inc}
    ${pict_api_src}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
  FILES
    ${pict_api_inc}
    ${pict_api_src}
)

target_include_directories(${target_name}
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${target_name}
  PRIVATE
    PictBuildOptions
)

set_target_properties(${target_name}
  PROPERTIES
    FOLDER src
    OUTPUT_NAME ${pict_api_output}
)
