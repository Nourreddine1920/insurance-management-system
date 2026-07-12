cmake_minimum_required(VERSION 3.15)
project(InsuranceManagementSystem C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Out-of-source builds enforcement
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds are not allowed. Please create a build directory and run cmake from there.")
endif()

# Find SQLite3 package
find_package(SQLite3 REQUIRED)

# Header search paths
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Compile options for code quality
if(MSVC)
    add_compile_options(/W4 /WX /wd4996) # Warning level 4, treat warnings as errors, disable MSVC deprecated warnings
else()
    add_compile_options(-Wall -Wextra -Werror -Wpedantic -Wshadow -Wstrict-prototypes -Wmissing-prototypes -fstack-protector-all)
endif()

# Source files definitions
set(SOURCES
    src/main.c
    src/cli.c
    src/validation.c
    src/database.c
    src/customer.c
    src/policy.c
    src/claim.c
    src/logger.c
    src/error.c
)

# Executable target
add_executable(insurance_system ${SOURCES})

# Link SQLite3 library
target_link_libraries(insurance_system PRIVATE SQLite::SQLite3)

# Enable CTest for unit test discovery
enable_testing()
add_subdirectory(tests)
