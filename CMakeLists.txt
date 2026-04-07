cmake_minimum_required(VERSION 3.15)
project(KVMControlApp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages provided by vcpkg
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ixwebsocket CONFIG REQUIRED)
find_package(LibDataChannel CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(FFMPEG REQUIRED)

# --- Source Files ---
add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/core/KVMApplication.cpp
    src/network/WinHttpClient.cpp
    src/network/WinNamedPipeClient.cpp
    src/video/RtpDepacketizer.cpp
    src/video/WebRTCStreamNode.cpp
    src/video/SDLVideoDecoder.cpp
    src/ui/OverlayGUI.cpp
    src/control/SDLInputCapturer.cpp
    src/control/WebSocketHIDClient.cpp
    src/control/EventMapper.cpp
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_LIBRARY_DIRS})

# Link Libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    SDL3::SDL3
    imgui::imgui
    ${FFMPEG_LIBRARIES}
    nlohmann_json::nlohmann_json
    ixwebsocket::ixwebsocket
    LibDataChannel::LibDataChannel
    d3d11.lib
    dxgi.lib
    ws2_32.lib
    crypt32.lib
    secur32.lib
    winhttp.lib
)
