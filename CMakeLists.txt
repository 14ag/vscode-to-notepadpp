cmake_minimum_required(VERSION 3.20)
project(VSCodeKeymapNpp VERSION 0.2.0 LANGUAGES CXX RC)

configure_file(
  src/VSCodeKeymapNpp.rc.in
  ${CMAKE_CURRENT_BINARY_DIR}/VSCodeKeymapNpp.rc
  @ONLY
)

add_library(VSCodeKeymapNpp SHARED
  src/VSCodeKeymapPlugin.cpp
  src/handlers/ActionHandlers.cpp
  src/handlers/CommentHandlers.cpp
  src/handlers/CursorHandlers.cpp
  src/handlers/FoldHandlers.cpp
  src/handlers/LineHandlers.cpp
  src/handlers/ScrollHandlers.cpp
  src/GeneratedBindings.inc
  ${CMAKE_CURRENT_BINARY_DIR}/VSCodeKeymapNpp.rc
)

target_include_directories(VSCodeKeymapNpp PRIVATE include src)
target_compile_features(VSCodeKeymapNpp PRIVATE cxx_std_20)
target_compile_definitions(VSCodeKeymapNpp PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(VSCodeKeymapNpp PRIVATE comctl32)

set_target_properties(VSCodeKeymapNpp PROPERTIES
  OUTPUT_NAME "VSCodeKeymapNpp"
  PREFIX ""
)

if(MSVC)
  target_compile_options(VSCodeKeymapNpp PRIVATE /W4 /permissive-)
endif()
