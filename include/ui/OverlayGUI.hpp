#pragma once
#include <cstdint>
#include <string>

struct SDL_Window;
struct SDL_Renderer;
#include <SDL3/SDL_events.h>

namespace kvm::ui {

class OverlayGUI {
public:
    OverlayGUI(SDL_Window* window, SDL_Renderer* renderer);
    ~OverlayGUI();

    void ProcessEvent(const SDL_Event* event);
    void NewFrame();
    void Render(uint32_t stream_width, uint32_t stream_height, const std::string& current_url, bool& is_running);

    // Update telemetry for debug display
    void UpdateLastKey(uint32_t scancode);

private:
    SDL_Renderer* m_renderer = nullptr;
    uint32_t m_lastKey{0};
};

} // namespace kvm::ui
