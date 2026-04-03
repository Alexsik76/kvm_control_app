#pragma once

#include <string>
#include <cstdint>

struct SDL_Window;
struct SDL_Renderer;
union SDL_Event;

namespace kvm::ui {

class OverlayGUI {
public:
    OverlayGUI(SDL_Window* window, SDL_Renderer* renderer);
    ~OverlayGUI();

    void ProcessEvent(const SDL_Event* event);
    void NewFrame();
    void Render(uint32_t stream_width, uint32_t stream_height, const std::string& current_url, bool& out_should_exit);

private:
    SDL_Renderer* m_renderer = nullptr;
};

} // namespace kvm::ui
