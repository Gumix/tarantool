macro(libnanoarrow_build)
    set(NANOARROW_INCLUDE_DIRS
        ${PROJECT_SOURCE_DIR}/third_party/arrow/nanoarrow/src/)
    set(NANOARROW_LIBRARIES nanoarrow nanoarrow_ipc)

    set(NANOARROW_IPC ON)
    file(MAKE_DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/third_party/arrow/nanoarrow/build/)
    add_subdirectory(
        ${PROJECT_SOURCE_DIR}/third_party/arrow/nanoarrow/
        ${CMAKE_CURRENT_BINARY_DIR}/third_party/arrow/nanoarrow/build/)

    # Silence "src/nanoarrow/ipc/decoder.c:1115:7: error: conversion from
    # `long unsigned int' to `int32_t' {aka `int'} may change value".
    set_target_properties(nanoarrow_ipc PROPERTIES COMPILE_FLAGS
        "${DEPENDENCY_CFLAGS} -Wno-error=conversion")
endmacro()
