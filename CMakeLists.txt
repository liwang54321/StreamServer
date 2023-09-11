cmake_minimum_required(VERSION 3.0.0)
project(device_develop VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_CXX_COMPILER_VERSION "9.2")

set(TOOLCHAIN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../toolchain/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu")
set(CROSS_COMPILE "${TOOLCHAIN_PATH}/bin/aarch64-none-linux-gnu-")

set(target_arch aarch64-linux-gnu)

# Configure cmake to look for libraries, include directories and
# packages inside the target root prefix.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_FIND_ROOT_PATH "/usr/${target_arch}" "${TOOLCHAIN_PATH}/aarch64-none-linux-gnu/libc")

set(CMAKE_C_COMPILER ${CROSS_COMPILE}gcc)
set(CMAKE_CXX_COMPILER ${CROSS_COMPILE}g++)
set(CMAKE_AR ${CROSS_COMPILE}ar CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB ${CROSS_COMPILE}ranlib)
set(CMAKE_LINKER ${CROSS_COMPILE}ld)

# needed to avoid doing some more strict compiler checks that
# are failing when cross-compiling
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_EXE_LINKER_FLAGS_INIT -Wl,--allow-shlib-undefined)

execute_process(COMMAND conan install ${CMAKE_CURRENT_SOURCE_DIR}/conanfile.py
     -pr:h ${CMAKE_CURRENT_SOURCE_DIR}/tools/conan_profile_aarch64
     -pr:b ${CMAKE_CURRENT_SOURCE_DIR}/tools/conan_profile_default
     --build missing)


include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}")

find_package(OpenCV REQUIRED)

add_executable(device_develop main.cpp)


target_sources(${PROJECT_NAME} PRIVATE src/VideoCapture.cpp)
target_sources(${PROJECT_NAME} PRIVATE src/VideoEncoder.cpp)
target_sources(${PROJECT_NAME} PRIVATE src/StreamServer.cpp)
target_sources(${PROJECT_NAME} PRIVATE src/StreamSink.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(${PROJECT_NAME} PRIVATE sdk/mpp/include)
target_include_directories(${PROJECT_NAME} PRIVATE sdk/ZLMediaKit/include)

target_link_directories(${PROJECT_NAME} PRIVATE sdk/mpp/lib)
target_link_directories(${PROJECT_NAME} PRIVATE sdk/ZLMediaKit/lib)
target_link_libraries(${PROJECT_NAME} PRIVATE rockchip_mpp mk_api)


target_link_libraries(${PROJECT_NAME} PRIVATE CONAN_PKG::magic_enum CONAN_PKG::spdlog CONAN_PKG::readerwriterqueue)
target_link_directories(${PROJECT_NAME} PRIVATE ${TOOLCHAIN_PATH}/aarch64-none-linux-gnu/lib64)
target_link_directories(${PROJECT_NAME} PRIVATE ${TOOLCHAIN_PATH}/aarch64-none-linux-gnu/libc/lib64)
target_link_directories(${PROJECT_NAME} PRIVATE ${TOOLCHAIN_PATH}/aarch64-none-linux-gnu/libc/usr/lib64)
# target_link_libraries(${PROJECT_NAME} PRIVATE CONAN_PKG::ffmpeg opencv::opencv_videoio opencv::opencv_highgui)
