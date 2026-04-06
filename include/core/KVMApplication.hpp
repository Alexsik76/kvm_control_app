#pragma once

#include <memory>
#include <string>
#include <SDL3/SDL.h>

// Forward declarations
namespace kvm::video { class IVideoDecoder; }
namespace kvm::control { class IInputCapturer; class IHIDClient; class IEventMapper; }
namespace kvm::ui { class OverlayGUI; }

namespace kvm::core {

class KVMApplication {
public:
    KVMApplication();
    ~KVMApplication();

    /**
     * @brief Initialize all systems (SDL, Video, HID, UI).
     */
    bool Initialize(const std::string& streamUrl, const std::string& hidUrl);

    /**
     * @brief Run the main application loop.
     */
    void Run();

private:
    void HandleEvents();
    void Render();

private:
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;

    // Independent Modules
    std::unique_ptr<video::IVideoDecoder> m_videoModule;
    std::unique_ptr<control::IInputCapturer> m_inputCapturer;
    std::unique_ptr<control::IHIDClient> m_hidModule;
    std::unique_ptr<control::IEventMapper> m_eventMapper;
    std::unique_ptr<ui::OverlayGUI> m_uiModule;

    std::string m_streamUrl;
    bool m_running = true;
    bool m_isCaptured = false;
};

} // namespace kvm::core
