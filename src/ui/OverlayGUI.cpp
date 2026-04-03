#include "ui/OverlayGUI.hpp"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>

namespace kvm::ui {

OverlayGUI::OverlayGUI(SDL_Window* window, SDL_Renderer* renderer)
    : m_renderer(renderer) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
}

OverlayGUI::~OverlayGUI() {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void OverlayGUI::ProcessEvent(const SDL_Event* event) {
    ImGui_ImplSDL3_ProcessEvent(event);
}

void OverlayGUI::NewFrame() {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void OverlayGUI::Render(uint32_t stream_width, uint32_t stream_height, const std::string& current_url, bool& out_should_exit) {
    ImGui::SetNextWindowPos({10, 10}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::Begin("KVM Info", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);

    if (stream_width > 0) {
        ImGui::TextColored({0.2f, 1.0f, 0.2f, 1}, "[OK] Streaming");
        ImGui::Text("Resolution : %ux%u", stream_width, stream_height);
    } else {
        ImGui::TextColored({1.0f, 0.6f, 0.0f, 1}, "Waiting for stream...");
        ImGui::Text("URL: %s", current_url.c_str());
    }
    
    if (ImGui::Button("Exit")) {
        out_should_exit = true;
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);
}

} // namespace kvm::ui
