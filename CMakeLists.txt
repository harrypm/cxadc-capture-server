cmake_minimum_required(VERSION 3.14)
project(cxadc-capture-server C)

set(CMAKE_C_STANDARD 11)

add_executable(cxadc-capture-server
        src/main.c
        src/http.c
        src/files.c)

target_compile_options(cxadc-capture-server PRIVATE
        -Wall
        -Wpedantic
        -Wextra)

target_compile_definitions(cxadc-capture-server PRIVATE
        CXADC_CAPTURE_SERVER_MAJOR=1
        CXADC_CAPTURE_SERVER_MINOR=4
        CXADC_CAPTURE_SERVER_PATCH=0
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED alsa)

target_include_directories(cxadc-capture-server PRIVATE ${ALSA_INCLUDE_DIRS})
target_link_directories(cxadc-capture-server PRIVATE ${ALSA_LIBRARY_DIRS})
target_compile_options(cxadc-capture-server PRIVATE ${ALSA_CFLAGS_OTHER})

target_link_libraries(cxadc-capture-server PRIVATE
        ${ALSA_LIBRARIES}
        pthread)
