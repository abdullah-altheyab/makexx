# Wraps a source file's content in a C++ raw string literal.
# Usage: cmake -DINPUT=<path> -DOUTPUT=<path> -DVAR=<identifier> -P embed_as_string.cmake
file(READ "${INPUT}" CONTENT)
file(WRITE "${OUTPUT}"
    "static const char ${VAR}[] = R\"ATMAKE(\n${CONTENT}\n)ATMAKE\";\n"
)
