#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include "video/IHardwareDecoder.hpp"
#include "network/INetworkReceiver.hpp"
#include <iostream>
#include <mutex>
#include <vector>

/**
 * @brief Main application entry point.
 * Orchestrates network reception, hardware decoding, and rendering.
 */
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    SDL_SetMainReady();

    // 1. Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // 2. Create Window and Renderer
    SDL_Window* window = SDL_CreateWindow(
        "IP-KVM Client MVP - UDP:5000",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // 3. Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // 4. Initialize Hardware Decoder
    auto decoder = kvm::video::CreateHardwareDecoder(renderer);
    if (!decoder->Initialize()) {
        std::cerr << "Failed to initialize hardware decoder. Check FFmpeg/D3D11 support." << std::endl;
    }

    // 5. Initialize Network Receiver (UDP port 5000)
    std::mutex data_mutex;
    std::vector<uint8_t> frame_buffer;

    auto receiver = kvm::network::CreateUDPReceiver();
    receiver->SetCallback([&](std::span<const uint8_t> data) {
        std::lock_guard<std::mutex> lock(data_mutex);
        frame_buffer.insert(frame_buffer.end(), data.begin(), data.end());
    });

    if (!receiver->Start(5000)) {
        std::cerr << "Failed to start UDP receiver on port 5000" << std::endl;
    }

    // 6. Main Loop
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
        }

        // Process incoming video data
        std::vector<uint8_t> local_buffer;
        {
            std::lock_guard<std::mutex> lock(data_mutex);
            if (!frame_buffer.empty()) {
                local_buffer = std::move(frame_buffer);
                frame_buffer.clear();
            }
        }

        if (!local_buffer.empty()) {
            // TODO: Implement a Jitter Buffer here to handle packet reordering and network spikes.
            // TODO: Use PTS (Presentation Timestamp) for smooth frame delivery timing.
            // Submit raw H.264 packet to decoder
            decoder->SubmitPacket(local_buffer, 0);
        }

        // UI Rendering
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("KVM Monitor");
        ImGui::Text("UDP Port: 5000");
        ImGui::Text("Resolution: %dx%d", decoder->GetWidth(), decoder->GetHeight());
        if (ImGui::Button("Exit")) done = true;
        ImGui::End();

        ImGui::Render();

        // Scene Rendering
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        // Draw decoded video texture (Zero-Copy handle)
        void* video_texture = decoder->GetTexture();
        if (video_texture) {
            SDL_RenderCopy(renderer, static_cast<SDL_Texture*>(video_texture), nullptr, nullptr);
        }

        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    receiver->Stop();
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
