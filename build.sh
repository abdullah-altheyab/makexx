#!/bin/sh
# Build makexx without cmake — just needs a C++ compiler.
# Mirrors what CMake does: embed the DSL header, the starter, and the vendored
# graph-viewer assets as C++ raw-string literals, then compile src/makexx.cpp.
# Run from the repository root.
set -e

CXX="${CXX:-g++}"
PREFIX="${PREFIX:-/usr/local}"

# Single source of truth for the version: the CMake project() line.
MAKEXX_VERSION=$(sed -n 's/^project(makexx VERSION \([0-9][0-9.]*\).*/\1/p' CMakeLists.txt)
: "${MAKEXX_VERSION:=0.0.0}"

# Wrap a file's bytes in a C++ raw string literal (matches cmake/embed_as_string.cmake).
#   embed <VAR> <SRC> <OUT>
embed() {
    printf 'static const char %s[] = R"ATMAKE(\n' "$1" > "$3"
    cat "$2" >> "$3"
    printf '\n)ATMAKE";\n' >> "$3"
}

echo "Embedding sources + graph assets..."
# Keep the VAR names and asset list in sync with CMakeLists.txt.
embed makefile_hpp_content     include/makexxfile.hpp                          makexxfile_embed.hpp
embed makefile_example_content src/starter.cpp                                 starter_embed.hpp
embed graph_dagre_js           assets/vendor/dagre.min.js                      graph_dagre_js_embed.hpp
embed graph_cytoscape_js       assets/vendor/cytoscape.min.js                  graph_cytoscape_js_embed.hpp
embed graph_cytoscape_dagre_js assets/vendor/cytoscape-dagre.min.js            graph_cytoscape_dagre_js_embed.hpp
embed graph_expand_collapse_js assets/vendor/cytoscape-expand-collapse.min.js  graph_expand_collapse_js_embed.hpp
embed graph_cytoscape_svg_js   assets/vendor/cytoscape-svg.min.js              graph_cytoscape_svg_js_embed.hpp
embed graph_viewer_html        assets/graph_viewer.html                        graph_viewer_html_embed.hpp

EMBEDS="makexxfile_embed.hpp starter_embed.hpp \
graph_dagre_js_embed.hpp graph_cytoscape_js_embed.hpp graph_cytoscape_dagre_js_embed.hpp \
graph_expand_collapse_js_embed.hpp graph_cytoscape_svg_js_embed.hpp graph_viewer_html_embed.hpp"

echo "Compiling makexx $MAKEXX_VERSION with $CXX..."
$CXX -std=c++17 -O2 -DMAKEXX_VERSION=\"$MAKEXX_VERSION\" -I include -I . src/makexx.cpp -o makexx

rm -f $EMBEDS

echo "Done (makexx $MAKEXX_VERSION). Install with: sudo cp makexx ${PREFIX}/bin/"
echo "  or run directly: ./makexx"
