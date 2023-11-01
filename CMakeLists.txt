cmake_minimum_required(VERSION 3.15.0)
project(StreamServer VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})

find_package(OpenCV REQUIRED)
find_package(ffmpeg REQUIRED)
find_package(magic_enum REQUIRED)
find_package(readerwriterqueue REQUIRED)
find_package(spdlog REQUIRED)
find_package(zlmediakit REQUIRED)
find_package(mpp REQUIRED)
find_package(libsrtp REQUIRED)
find_package(usrsctp REQUIRED)

add_executable(${PROJECT_NAME} application/main.cpp)

target_sources(${PROJECT_NAME} PRIVATE 
    application/sources/VideoCapture.cpp
    application/sources/VideoEncoder.cpp
    application/sources/StreamServer.cpp
    application/sources/StreamSink.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE application/include)

message(STATUS "${zlmediakit_INCLUDE_DIRS} ${zlmediakit_LIBRARIES}")
message(STATUS "${mpp_INCLUDE_DIRS} ${mpp_LIBRARIES}")

target_link_libraries(${PROJECT_NAME} PUBLIC 
    magic_enum::magic_enum 
    spdlog::spdlog_header_only
    zlmediakit::zlmediakit
    mpp::mpp 
    libsrtp::libsrtp 
    openssl::openssl 
    usrsctp::usrsctp
    ffmpeg::ffmpeg
    readerwriterqueue::readerwriterqueue
)

# target_link_libraries(${PROJECT_NAME} PRIVATE CONAN_PKG::ffmpeg opencv::opencv_videoio opencv::opencv_highgui)
