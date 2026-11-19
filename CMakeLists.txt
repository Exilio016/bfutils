cmake_minimum_required(VERSION 3.20)

project(bfutils VERSION 0.1.0 LANGUAGES C)

# C11 with GNU extensions (typeof, statement expressions, cleanup attribute).
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

add_compile_options(-Wall -Wextra)

# Coverage instrumentation (enabled only via -DBFU_COVERAGE=ON, used by the
# `coverage` target).  Keeps the normal build free of gcov profiling code.
option(BFU_COVERAGE "Build with gcov coverage instrumentation" OFF)
if(BFU_COVERAGE)
    add_compile_options(-fprofile-arcs -ftest-coverage)
    add_link_options(--coverage)
endif()

# -----------------------------------------------------------------------------
# Library (static + shared, mirroring the old Makefile's `lib` target)
# -----------------------------------------------------------------------------
set(LIB_SRCS
    src/bfu_string.c
    src/bfu_array.c
    src/bfu_map.c
    src/bfu_process.c
)

add_library(bfutils STATIC ${LIB_SRCS})
target_include_directories(bfutils PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
set_target_properties(bfutils PROPERTIES
    OUTPUT_NAME bfutils
    SOVERSION ${PROJECT_VERSION_MAJOR}
    VERSION   ${PROJECT_VERSION}
)

add_library(bfutils_shared SHARED ${LIB_SRCS})
target_include_directories(bfutils_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
set_target_properties(bfutils_shared PROPERTIES
    OUTPUT_NAME bfutils
    SOVERSION ${PROJECT_VERSION_MAJOR}
    VERSION   ${PROJECT_VERSION}
)

# Convenience alias so in-tree targets can link bfutils::bfutils.
add_library(bfutils::bfutils ALIAS bfutils)

# -----------------------------------------------------------------------------
# Test suite (fault injection via the linker --wrap option)
# -----------------------------------------------------------------------------
set(TEST_SRCS
    tests/test_main.c
    tests/test_framework.c
    tests/test_array.c
    tests/test_string.c
    tests/test_map.c
    tests/test_process.c
)

add_executable(bfu_test ${TEST_SRCS})
target_link_libraries(bfu_test PRIVATE bfutils)
target_link_options(bfu_test PRIVATE
    -Wl,--wrap=malloc
    -Wl,--wrap=realloc
    -Wl,--wrap=vsnprintf
    -Wl,--wrap=free
)

# -----------------------------------------------------------------------------
# Convenience targets
# -----------------------------------------------------------------------------
add_custom_target(test
    COMMAND ${CMAKE_COMMAND} --build . --target bfu_test
    COMMAND ${CMAKE_BINARY_DIR}/bfu_test string
    COMMAND ${CMAKE_BINARY_DIR}/bfu_test array
    COMMAND ${CMAKE_BINARY_DIR}/bfu_test map
    COMMAND ${CMAKE_BINARY_DIR}/bfu_test process
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running bfutils test suites (string, array, map, process)"
    VERBATIM
)

# Coverage build: instrument with gcov, run the full suite, and enforce 100%
# line/branch coverage on src/*.c (mirrors the old `make coverage`).
add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}
        -DBFU_COVERAGE=ON
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test
    COMMAND ${CMAKE_COMMAND} -E env BUILD_DIR=${CMAKE_BINARY_DIR}
        python3 ${CMAKE_SOURCE_DIR}/tests/check_coverage.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Coverage build (gcov): instrument, run tests, enforce 100% coverage"
    VERBATIM
)

# -----------------------------------------------------------------------------
# Install: headers, static + shared libs, pkg-config and cmake package files
# -----------------------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS bfutils bfutils_shared
    EXPORT bfutilsTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# pkg-config file
configure_file(bfutils.pc.in ${PROJECT_NAME}.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# cmake package config (find_package(bfutils) / bfutils::bfutils)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/bfutilsConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(bfutilsConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/bfutilsConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/bfutilsConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/bfutilsConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(EXPORT bfutilsTargets
    NAMESPACE bfutils::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
