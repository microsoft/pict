add_test(
  NAME run_pict
  COMMAND perl test.pl $<TARGET_FILE:pict_cli> $<IF:$<CONFIG:Debug>,dbg,rel>.log
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
