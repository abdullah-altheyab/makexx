cmake_minimum_required(VERSION 3.15)
project(makexx VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EMBED_SCRIPT ${CMAKE_SOURCE_DIR}/cmake/embed_as_string.cmake)

add_custom_command(
    OUTPUT  ${CMAKE_BINARY_DIR}/makexxfile_embed.hpp
    COMMAND ${CMAKE_COMMAND}
                -DINPUT=${CMAKE_SOURCE_DIR}/include/makexxfile.hpp
                -DOUTPUT=${CMAKE_BINARY_DIR}/makexxfile_embed.hpp
                -DVAR=makefile_hpp_content
                -P ${EMBED_SCRIPT}
    DEPENDS ${CMAKE_SOURCE_DIR}/include/makexxfile.hpp
    COMMENT "Embedding include/makexxfile.hpp"
)

add_custom_command(
    OUTPUT  ${CMAKE_BINARY_DIR}/starter_embed.hpp
    COMMAND ${CMAKE_COMMAND}
                -DINPUT=${CMAKE_SOURCE_DIR}/src/starter.cpp
                -DOUTPUT=${CMAKE_BINARY_DIR}/starter_embed.hpp
                -DVAR=makefile_example_content
                -P ${EMBED_SCRIPT}
    DEPENDS ${CMAKE_SOURCE_DIR}/src/starter.cpp
    COMMENT "Embedding src/starter.cpp"
)

# Interactive dependency-graph viewer assets: the vendored Cytoscape stack
# plus our viewer template are embedded as string literals so `makexx
# --build-graph` can assemble a standalone, offline, single-file HTML graph.
set(GRAPH_ASSETS
    "graph_dagre_js|assets/vendor/dagre.min.js"
    "graph_cytoscape_js|assets/vendor/cytoscape.min.js"
    "graph_cytoscape_dagre_js|assets/vendor/cytoscape-dagre.min.js"
    "graph_expand_collapse_js|assets/vendor/cytoscape-expand-collapse.min.js"
    "graph_cytoscape_svg_js|assets/vendor/cytoscape-svg.min.js"
    "graph_viewer_html|assets/graph_viewer.html"
)
set(GRAPH_EMBED_HEADERS)
foreach(spec IN LISTS GRAPH_ASSETS)
    string(REPLACE "|" ";" parts "${spec}")
    list(GET parts 0 gvar)
    list(GET parts 1 grel)
    set(gout ${CMAKE_BINARY_DIR}/${gvar}_embed.hpp)
    add_custom_command(
        OUTPUT  ${gout}
        COMMAND ${CMAKE_COMMAND}
                    -DINPUT=${CMAKE_SOURCE_DIR}/${grel}
                    -DOUTPUT=${gout}
                    -DVAR=${gvar}
                    -P ${EMBED_SCRIPT}
        DEPENDS ${CMAKE_SOURCE_DIR}/${grel}
        COMMENT "Embedding ${grel}"
    )
    list(APPEND GRAPH_EMBED_HEADERS ${gout})
endforeach()

add_executable(makexx
    src/makexx.cpp
    ${CMAKE_BINARY_DIR}/makexxfile_embed.hpp
    ${CMAKE_BINARY_DIR}/starter_embed.hpp
    ${GRAPH_EMBED_HEADERS}
)
target_include_directories(makexx PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}
)
target_compile_definitions(makexx PRIVATE MAKEXX_VERSION="${PROJECT_VERSION}")

install(TARGETS makexx RUNTIME DESTINATION bin)

enable_testing()

add_executable(test_dsl tests/test_dsl.cpp)
target_include_directories(test_dsl PRIVATE ${CMAKE_SOURCE_DIR}/include)
set_target_properties(test_dsl PROPERTIES CXX_STANDARD 17)

add_test(NAME dsl COMMAND test_dsl)
