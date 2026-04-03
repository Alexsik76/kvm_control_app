cmake_minimum_required(VERSION 3.15)
project(KVMControlApp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

# --- SDL2 setup ---
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.x
)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2)

# --- Dear ImGui setup ---
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.4
)
FetchContent_MakeAvailable(imgui)

# --- FFmpeg setup (Using EXACT URL provided by user) ---
FetchContent_Declare(
    ffmpeg
    URL https://github.com/GyanD/codexffmpeg/releases/download/8.1/ffmpeg-8.1-full_build-shared.zip
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)
FetchContent_MakeAvailable(ffmpeg)

# Automatically find the root directory (handles nested zip structures)
if(EXISTS "${ffmpeg_SOURCE_DIR}/bin")
    set(FFMPEG_ROOT_DIR "${ffmpeg_SOURCE_DIR}")
else()
    file(GLOB FFMPEG_CANDIDATES LIST_DIRECTORIES true "${ffmpeg_SOURCE_DIR}/ffmpeg-*")
    list(LENGTH FFMPEG_CANDIDATES FFMPEG_COUNT)
    if(FFMPEG_COUNT EQUAL 0)
        set(FFMPEG_ROOT_DIR "${ffmpeg_SOURCE_DIR}")
    else()
        list(GET FFMPEG_CANDIDATES 0 FFMPEG_ROOT_DIR)
    endif()
endif()

set(FFMPEG_INCLUDE_DIR "${FFMPEG_ROOT_DIR}/include")
set(FFMPEG_LIB_DIR "${FFMPEG_ROOT_DIR}/lib")
set(FFMPEG_BIN_DIR "${FFMPEG_ROOT_DIR}/bin")

# --- Source Files ---
add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/video/FFmpegStreamNode.cpp
    src/video/SDLVideoDecoder.cpp
    src/ui/OverlayGUI.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE 
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${FFMPEG_INCLUDE_DIR}
)

# Link Libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    SDL2::SDL2 
    SDL2::SDL2main 
    d3d11.lib
    dxgi.lib
)

set(FFMPEG_LIBS avcodec avformat avutil swscale)
foreach(LIB ${FFMPEG_LIBS})
    target_link_libraries(${PROJECT_NAME} PRIVATE "${FFMPEG_LIB_DIR}/${LIB}.lib")
endforeach()

# Post-build DLL Copy
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${FFMPEG_BIN_DIR}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()
