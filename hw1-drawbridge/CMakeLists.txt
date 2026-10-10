cmake_minimum_required(VERSION 3.15)

project(bridge LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

option(BRIDGE_SANITIZE "Enable AddressSanitizer and UBSan" OFF)

set(BRIDGE_SOURCES
    src/bridge.c
    src/config.c
    src/event.c
    src/logger.c
    src/operator.c
    src/queue.c
    src/rng.c
    src/signals.c
    src/sim.c
    src/stream.c
    src/transport.c
    src/main.c
)

add_executable(bridge ${BRIDGE_SOURCES})

target_include_directories(bridge PRIVATE src)
target_link_libraries(bridge PRIVATE m)
target_compile_options(bridge PRIVATE -Wall -Wextra)

target_compile_definitions(bridge PRIVATE _DEFAULT_SOURCE)

if(BRIDGE_SANITIZE)
    target_compile_options(bridge PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(bridge PRIVATE -fsanitize=address,undefined)
endif()

add_custom_target(run
    COMMAND $<TARGET_FILE:bridge>
    DEPENDS bridge
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    USES_TERMINAL
)

add_custom_target(run-args
    COMMAND $<TARGET_FILE:bridge> ${ARGS}
    DEPENDS bridge
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    USES_TERMINAL
)

add_custom_target(clean-log
    COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/sim.log
)
