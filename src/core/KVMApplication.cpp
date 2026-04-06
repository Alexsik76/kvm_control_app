#include "core/KVMApplication.hpp"
#include "video/IVideoDecoder.hpp"
#include "control/IInputCapturer.hpp"
#include "control/IHIDClient.hpp"
#include "control/IEventMapper.hpp"
#include "ui/OverlayGUI.hpp"
#include "network/WinHttpClient.hpp"
#include <ixwebsocket/IXNetSystem.h>
#include <iostream>
#include <imgui.h>
#ifdef _WIN32
#include <winsock2.h>
#endif

namespace kvm::core {

KVMApplication::KVMApplication() {
    ix::initNetSystem();
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

KVMApplication::~KVMApplication() {
    ix::uninitNetSystem();
#ifdef _WIN32
    WSACleanup();
#endif
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

bool KVMApplication::Initialize(const std::string& streamUrl, const std::string& hidUrl) {
    m_streamUrl = streamUrl;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        std::cerr << "[SDL] Init error: " << SDL_GetError() << "\n";
        return false;
    }

    m_window = SDL_CreateWindow("IP-KVM Client", 1280, 720, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!m_window) return false;

    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (!m_renderer) return false;

    // 0. Initialize Network
    auto httpClient = std::make_shared<network::WinHttpClient>();

    // 1. Initialize Video Module
    m_videoModule = video::CreateVideoDecoder(m_renderer, httpClient);
    if (!m_videoModule->Initialize()) return false;
    m_videoModule->OpenStream(streamUrl);

    // 2. Initialize UI Module FIRST (to avoid nullptr in callbacks)
    m_uiModule = std::make_unique<ui::OverlayGUI>(m_window, m_renderer);

    // 3. Initialize HID Module
    m_inputCapturer = control::CreateInputCapturer(m_window);
    m_hidModule = control::CreateHIDClient();
    m_hidModule->Connect(hidUrl);
    m_eventMapper = control::CreateEventMapper();

    // Link Input to HID and UI (Telemetry)
    m_inputCapturer->SetKeyboardCallback([this](const auto& ev) { 
        if (!ev.keys.empty()) m_uiModule->UpdateLastKey(ev.keys[0]);
        
        // Translate SDL Scancodes and Modifiers to HID reports
        auto hidEvent = ev;
        for (auto& key : hidEvent.keys) {
            key = m_eventMapper->SDLToHIDKeyboard(key);
        }
        hidEvent.modifiers = m_eventMapper->SDLToHIDModifiers(ev.modifiers);

        m_hidModule->SendKeyboardEvent(hidEvent); 
    });

    m_inputCapturer->SetMouseCallback([this](const auto& ev) { 
        // Forward relative deltas directly to HID
        m_hidModule->SendMouseEvent(ev); 
    });

    return true;
}

void KVMApplication::Run() {
    while (m_running) {
        HandleEvents();
        Render();
    }
}

void KVMApplication::HandleEvents() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        // Forward event to UI
        m_uiModule->ProcessEvent(&ev);
        
        if (ev.type == SDL_EVENT_QUIT) {
            m_running = false;
        }

        if (m_isCaptured) {
            // Hotkey to release capture: Right Ctrl
            if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.scancode == SDL_SCANCODE_RCTRL) {
                m_isCaptured = false;
                m_inputCapturer->SetCaptureEnabled(false);
                SDL_SetWindowFullscreen(m_window, false); // Disable fullscreen
                continue; // Do not forward this key to HID
            }
            
            // Ignore the Right Ctrl release as well to avoid stuck keys on remote
            if (ev.type == SDL_EVENT_KEY_UP && ev.key.scancode == SDL_SCANCODE_RCTRL) {
                continue;
            }

            // Forward input to HID Capturer only if captured
            m_inputCapturer->ProcessEvent(&ev);
        } else {
            // Click to capture (only if ImGui didn't process the click)
            if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                ImGuiIO& io = ImGui::GetIO();
                if (!io.WantCaptureMouse) {
                    m_isCaptured = true;
                    m_inputCapturer->SetCaptureEnabled(true);
                    SDL_SetWindowFullscreen(m_window, true);
                }
            }
        }
    }
}

void KVMApplication::Render() {
    m_uiModule->NewFrame();

    SDL_SetRenderDrawColor(m_renderer, 20, 20, 20, 255);
    SDL_RenderClear(m_renderer);

    // Draw Video
    void* tex = m_videoModule->GetTexture();
    if (tex) {
        SDL_RenderTexture(m_renderer, static_cast<SDL_Texture*>(tex), nullptr, nullptr);
    }

    // Draw UI
    m_uiModule->Render(m_videoModule->GetWidth(), m_videoModule->GetHeight(), m_streamUrl, m_running,
                       m_videoModule->IsConnected(), m_hidModule->IsConnected(), m_isCaptured);

    SDL_RenderPresent(m_renderer);
}

} // namespace kvm::core
