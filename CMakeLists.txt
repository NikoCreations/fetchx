cmake_minimum_required(VERSION 3.16)

project(fetchx VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(fetchx src/fetchx.cpp)
add_executable(watchurl src/watchurl.cpp)

target_compile_options(fetchx PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(watchurl PRIVATE -Wall -Wextra -Wpedantic)

target_compile_definitions(fetchx PRIVATE FETCHX_VERSION="${PROJECT_VERSION}")
target_compile_definitions(watchurl PRIVATE FETCHX_VERSION="${PROJECT_VERSION}")

include(GNUInstallDirs)

install(TARGETS fetchx watchurl
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
