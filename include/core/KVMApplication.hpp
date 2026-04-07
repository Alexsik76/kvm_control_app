#pragma once

#include <memory>
#include <string>
#include <SDL3/SDL.h>

// Forward declarations
namespace kvm::video { class IVideoDecoder; }
namespace kvm::control { class IInputCapturer; class IHIDClient; class IEventMapper; }
namespace kvm::ui { class OverlayGUI; }

namespace kvm::network { class ILauncherClient; }

namespace kvm::core {

class KVMApplication {
public:
    KVMApplication();
    ~KVMApplication();

    /**
     * @brief Initialize with injected dependencies.
     */
    bool Initialize(
        const std::string& streamUrl,
        const std::string& hidUrl,
        std::unique_ptr<video::IVideoDecoder> videoModule,
        std::unique_ptr<control::IInputCapturer> inputCapturer,
        std::unique_ptr<control::IHIDClient> hidModule,
        std::unique_ptr<control::IEventMapper> eventMapper,
        network::ILauncherClient* launcher = nullptr
    );

    /**
     * @brief Run the main application loop.
     */
    void Run();

    SDL_Renderer* GetRenderer() const { return m_renderer.get(); }
    SDL_Window* GetWindow() const { return m_window.get(); }

private:
    void HandleEvents();
    void Render();

private:
    // RAII for SDL resources
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> m_window;
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> m_renderer;

    // Injected Modules
    std::unique_ptr<video::IVideoDecoder> m_videoModule;
    std::unique_ptr<control::IInputCapturer> m_inputCapturer;
    std::unique_ptr<control::IHIDClient> m_hidModule;
    std::unique_ptr<control::IEventMapper> m_eventMapper;
    std::unique_ptr<ui::OverlayGUI> m_uiModule;

    network::ILauncherClient* m_launcher = nullptr;
    std::string m_streamUrl;
    bool m_running = true;
    bool m_isCaptured = false;
};

} // namespace kvm::core
