#include "ui/OverlayGUI.hpp"
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <SDL.h>

namespace kvm::ui {

OverlayGUI::OverlayGUI(SDL_Window* window, SDL_Renderer* renderer)
    : m_renderer(renderer) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
}

OverlayGUI::~OverlayGUI() {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void OverlayGUI::ProcessEvent(const SDL_Event* event) {
    ImGui_ImplSDL2_ProcessEvent(event);
}

void OverlayGUI::NewFrame() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
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
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
}

} // namespace kvm::ui
