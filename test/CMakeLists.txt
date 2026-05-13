add_test(
  NAME run_pict
  COMMAND perl test.pl $<TARGET_FILE:pict_cli> $<IF:$<CONFIG:Debug>,dbg,rel>.log
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
  NAME run_pict_benchmark
  COMMAND $<TARGET_FILE:pict_benchmark> --runs 3 --benchmark 3^4
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
