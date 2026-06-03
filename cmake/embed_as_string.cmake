# Wraps a source file's content in a C++ raw string literal.
# Usage: cmake -DINPUT=<path> -DOUTPUT=<path> -DVAR=<identifier> -P embed_as_string.cmake
file(READ "${INPUT}" CONTENT)
# file(READ) stops at the first NUL byte, which would silently truncate the
# embedded string (and so the embedded asset at runtime). Catch that here: if
# the bytes we read are shorter than the file, a NUL (or other binary garbage)
# crept in. Fail the build with a clear message rather than ship a cut-off asset.
file(SIZE "${INPUT}" REAL_SIZE)
string(LENGTH "${CONTENT}" READ_SIZE)
if(NOT REAL_SIZE EQUAL READ_SIZE)
    message(FATAL_ERROR
        "embed_as_string: '${INPUT}' read ${READ_SIZE} of ${REAL_SIZE} bytes — "
        "it likely contains a NUL byte, which would silently truncate the embed.")
endif()
file(WRITE "${OUTPUT}"
    "static const char ${VAR}[] = R\"ATMAKE(\n${CONTENT}\n)ATMAKE\";\n"
)
