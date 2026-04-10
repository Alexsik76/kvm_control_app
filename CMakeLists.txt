cmake_minimum_required(VERSION 3.15)
project(KVMVideoCodec VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages provided by vcpkg
find_package(nlohmann_json CONFIG REQUIRED)
find_package(LibDataChannel CONFIG REQUIRED)
find_package(FFMPEG REQUIRED)

# --- Source Files ---
add_library(${PROJECT_NAME} SHARED
    src/KVMVideoCodec.cpp
    src/network/HttpClient.cpp
    src/video/RtpDepacketizer.cpp
    src/video/WebRTCStreamNode.cpp
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS KVM_EXPORT)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_LIBRARY_DIRS})

# Link Libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${FFMPEG_LIBRARIES}
    nlohmann_json::nlohmann_json
    LibDataChannel::LibDataChannel
    ws2_32.lib
    crypt32.lib
    secur32.lib
    winhttp.lib
    bcrypt.lib
)

# Output configuration
set_target_properties(${PROJECT_NAME} PROPERTIES 
    PREFIX "" 
    SUFFIX ".dll"
)
