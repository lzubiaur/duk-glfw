# Download dependencies and build the tests
cmake_minimum_required(VERSION 2.8)

project(duk_glfw_tests)

# Download GLFW
set(GLFW_DIST "glfw-3.1.2")
set(GLFW_URL https://github.com/glfw/glfw/releases/download/3.1.2/glfw-3.1.2.zip)
message(STATUS "Downloading ${GLFW_DIST}...")
file(DOWNLOAD ${GLFW_URL} ${CMAKE_BINARY_DIR}/${GLFW_DIST}.zip)
message(STATUS "Unpacking ${GLFW_DIST}...")
execute_process(COMMAND ${CMAKE_COMMAND} -E tar xfz ${GLFW_DIST}.zip
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_subdirectory(${CMAKE_BINARY_DIR}/${GLFW_DIST})

# Download Duktape
set(DUKTAPE_DIST "duktape-1.4.0")
set(DUKTAPE_URL http://duktape.org/${DUKTAPE_DIST}.tar.xz)
message(STATUS "Downloading ${DUKTAPE_DIST}...")
file(DOWNLOAD ${DUKTAPE_URL} ${CMAKE_BINARY_DIR}/${DUKTAPE_DIST}.tar.xz)
message(STATUS "Unpacking ${DUKTAPE_DIST}...")
execute_process(COMMAND ${CMAKE_COMMAND} -E tar xfz ${DUKTAPE_DIST}.tar.xz
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# Enable Duktape debug logs
# target_compile_definitions(duktape PRIVATE DUK_OPT_DEBUG=1 DUK_OPT_DPRINT=1 DUK_OPT_DPRINT_COLORS=1)

# set(CMAKE_C_FLAGS -std=c99 -pedantic-errors -pedantic)

include_directories(
  ./
  ${CMAKE_BINARY_DIR}/${DUKTAPE_DIST}/src
  ${CMAKE_BINARY_DIR}/${GLFW_DIST}/include)

add_executable(duk_glfw
  duk_glfw.c
  tests/test.c
  ${DUKTAPE_DIST}/src/duktape.c)
# http://duktape.org/guide.html#compiling
target_compile_options(duk_glfw PRIVATE -std=c99 -Os -fomit-frame-pointer -fstrict-aliasing)
target_link_libraries(duk_glfw glfw ${GLFW_LIBRARIES})

# Copy the Javascript file to the build tree
file(COPY tests/glfw.js DESTINATION ${CMAKE_BINARY_DIR})
