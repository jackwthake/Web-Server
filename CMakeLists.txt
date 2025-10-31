cmake_minimum_required(VERSION 3.10)
project(WebServer VERSION 1.0.0 LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")

# Get git commit hash
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# If not in a git repo, set to "unknown"
if(NOT GIT_COMMIT_HASH)
    set(GIT_COMMIT_HASH "unknown")
endif()

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/server.cpp
    src/util/log.cpp
    src/util/pool.cpp
)

# Create executable
add_executable(serve ${SOURCES})

# Include directories
target_include_directories(serve PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${OPENSSL_INCLUDE_DIR}
)

# Add compile definitions
target_compile_definitions(serve PRIVATE
    GIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
)

# Link libraries
target_link_libraries(serve PRIVATE
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Copy secret directory to build directory (if it exists)
if(EXISTS ${CMAKE_SOURCE_DIR}/secret)
    file(COPY ${CMAKE_SOURCE_DIR}/secret
         DESTINATION ${CMAKE_BINARY_DIR})
endif()

# Copy public directory to build directory
file(COPY ${CMAKE_SOURCE_DIR}/public
     DESTINATION ${CMAKE_BINARY_DIR})

# Copy config file to build directory (if it exists)
if(EXISTS ${CMAKE_SOURCE_DIR}/secure-serve.conf)
    file(COPY ${CMAKE_SOURCE_DIR}/secure-serve.conf
         DESTINATION ${CMAKE_BINARY_DIR})
endif()

# Installation rules
install(TARGETS serve DESTINATION bin)
install(FILES routing.conf DESTINATION bin)
if(EXISTS ${CMAKE_SOURCE_DIR}/secure-serve.conf)
    install(FILES secure-serve.conf DESTINATION bin)
endif()
if(EXISTS ${CMAKE_SOURCE_DIR}/secret)
    install(DIRECTORY secret DESTINATION bin)
endif()
install(DIRECTORY public DESTINATION bin)

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")
message(STATUS "Git commit: ${GIT_COMMIT_HASH}")
