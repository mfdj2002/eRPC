cmake_minimum_required(VERSION 3.10)
project(ServerProject)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

# Add executable
add_executable(server server.cc)

# Set compile options
target_compile_options(server PRIVATE
    -g
    -Wno-deprecated-declarations
    -march=native
)

# Set compile definitions
target_compile_definitions(server PRIVATE
    ERPC_DPDK=true
)

# Set include directories
target_include_directories(server PRIVATE
    ../src
    ../third_party/asio/include
    /usr/include/dpdk
    ${DPDK_INCLUDE_DIRS}
)

# Set link directories
target_link_directories(server PRIVATE
    ../build
)

# Set link libraries
target_link_libraries(server PRIVATE
    erpc
    pthread
    numa
    dl
    ibverbs
    mlx4
    mlx5
    ${DPDK_LIBRARIES}
)

# Set link options
target_link_options(server PRIVATE
    -Wl,--whole-archive
    ${DPDK_LDFLAGS}
    -Wl,--no-whole-archive
)
