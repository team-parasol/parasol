# SVG module

set (MOD svg)

idl_c ("${MOD}.fdl" NAME ${MOD}_defs
   OUTPUT "${INCLUDE_OUTPUT}/modules/${MOD}.h"
   ARGS "--files={" "class_rsvg.cpp" "class_svg.cpp" "class_svgimage.cpp" "}")

idl_def ("${MOD}.fdl" NAME ${MOD}_idl
   OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}_def.c")

add_library (${MOD} SHARED)

set_module_defaults (${MOD})

target_sources (${MOD} PRIVATE
   "${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/katana-parser/src/foundation.c"
   "${CMAKE_CURRENT_SOURCE_DIR}/katana-parser/src/katana.lex.c"
   "${CMAKE_CURRENT_SOURCE_DIR}/katana-parser/src/katana.tab.c"
   "${CMAKE_CURRENT_SOURCE_DIR}/katana-parser/src/parser.c"
   "${CMAKE_CURRENT_SOURCE_DIR}/katana-parser/src/selector.c"
   "${CMAKE_CURRENT_SOURCE_DIR}/katana-parser/src/tokenizer.c")

target_include_directories (${MOD} PRIVATE
   "${CMAKE_CURRENT_SOURCE_DIR}/katana-parser/src")

target_link_libraries (${MOD} PRIVATE "m")
