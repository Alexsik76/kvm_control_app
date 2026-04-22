cmake_minimum_required(VERSION 3.15)
project(KVMVideoCodec VERSION 0.1.0 LANGUAGES CXX)

# Version embedding
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE KVM_GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT KVM_GIT_HASH)
    set(KVM_GIT_HASH "unknown")
endif()
string(TIMESTAMP KVM_BUILD_DATE "%Y-%m-%dT%H:%M:%SZ" UTC)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Version.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/Version.hpp
    @ONLY
)

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
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
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
