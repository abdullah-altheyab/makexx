#!/bin/sh
# Build makexx without cmake — just needs a C++ compiler.
set -e

CXX="${CXX:-g++}"
PREFIX="${PREFIX:-/usr/local}"

echo "Embedding headers..."
printf 'static const char makefile_hpp_content[] = R"ATMAKE(\n' > makexxfile_embed.hpp
cat include/makexxfile.hpp >> makexxfile_embed.hpp
printf '\n)ATMAKE";\n' >> makexxfile_embed.hpp

printf 'static const char makefile_example_content[] = R"ATMAKE(\n' > starter_embed.hpp
cat src/starter.cpp >> starter_embed.hpp
printf '\n)ATMAKE";\n' >> starter_embed.hpp

echo "Compiling makexx with $CXX..."
$CXX -std=c++17 -O2 -DMAKEXX_VERSION=\"0.1.0\" -I include -I . src/makexx.cpp -o makexx

rm -f makexxfile_embed.hpp starter_embed.hpp

echo "Done. Install with: sudo cp makexx ${PREFIX}/bin/"
echo "  or run directly: ./makexx"
