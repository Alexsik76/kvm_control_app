#include "core/KVMApplication.hpp"
#include "video/IVideoDecoder.hpp"
#include "control/IInputCapturer.hpp"
#include "control/IHIDClient.hpp"
#include "control/IEventMapper.hpp"
#include "ui/OverlayGUI.hpp"
#include <ixwebsocket/IXNetSystem.h>
#include <iostream>
#include <imgui.h>
#ifdef _WIN32
#include <winsock2.h>
#endif

namespace kvm::core {

KVMApplication::KVMApplication() 
    : m_window(nullptr, SDL_DestroyWindow)
    , m_renderer(nullptr, SDL_DestroyRenderer) 
{
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
    SDL_Quit();
}

bool KVMApplication::Initialize(
    const std::string& streamUrl,
    const std::string& hidUrl,
    std::unique_ptr<video::IVideoDecoder> videoModule,
    std::unique_ptr<control::IInputCapturer> inputCapturer,
    std::unique_ptr<control::IHIDClient> hidModule,
    std::unique_ptr<control::IEventMapper> eventMapper) 
{
    m_streamUrl = streamUrl;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        std::cerr << "[SDL] Init error: " << SDL_GetError() << "\n";
        return false;
    }

    m_window.reset(SDL_CreateWindow("IP-KVM Client", 1280, 720, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY));
    if (!m_window) return false;

    m_renderer.reset(SDL_CreateRenderer(m_window.get(), nullptr));
    if (!m_renderer) return false;

    // Use Injected Modules
    m_videoModule = std::move(videoModule);
    m_inputCapturer = std::move(inputCapturer);
    m_hidModule = std::move(hidModule);
    m_eventMapper = std::move(eventMapper);

    // Link modules to SDL handles
    m_videoModule->SetRenderer(m_renderer.get());
    m_inputCapturer->SetWindow(m_window.get());

    // Initialize Video
    if (!m_videoModule->Initialize()) return false;
    m_videoModule->OpenStream(streamUrl);

    // Initialize UI Module
    m_uiModule = std::make_unique<ui::OverlayGUI>(m_window.get(), m_renderer.get());

    // Connect HID
    m_hidModule->Connect(hidUrl);

    // Link Input to HID and UI
    m_inputCapturer->SetKeyboardCallback([this](const auto& ev) { 
        if (!ev.keys.empty()) m_uiModule->UpdateLastKey(ev.keys[0]);
        
        auto hidEvent = ev;
        for (auto& key : hidEvent.keys) {
            key = m_eventMapper->SDLToHIDKeyboard(key);
        }
        hidEvent.modifiers = m_eventMapper->SDLToHIDModifiers(ev.modifiers);

        m_hidModule->SendKeyboardEvent(hidEvent); 
    });

    m_inputCapturer->SetMouseCallback([this](const auto& ev) { 
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
        m_uiModule->ProcessEvent(&ev);
        
        if (ev.type == SDL_EVENT_QUIT) {
            m_running = false;
        }

        if (m_isCaptured) {
            if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.scancode == SDL_SCANCODE_RCTRL) {
                m_isCaptured = false;
                m_inputCapturer->SetCaptureEnabled(false);
                SDL_SetWindowFullscreen(m_window.get(), false);
                continue;
            }
            
            if (ev.type == SDL_EVENT_KEY_UP && ev.key.scancode == SDL_SCANCODE_RCTRL) {
                continue;
            }

            m_inputCapturer->ProcessEvent(&ev);
        } else {
            if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                ImGuiIO& io = ImGui::GetIO();
                if (!io.WantCaptureMouse) {
                    m_isCaptured = true;
                    m_inputCapturer->SetCaptureEnabled(true);
                    SDL_SetWindowFullscreen(m_window.get(), true);
                }
            }
        }
    }
}

void KVMApplication::Render() {
    m_uiModule->NewFrame();

    SDL_SetRenderDrawColor(m_renderer.get(), 20, 20, 20, 255);
    SDL_RenderClear(m_renderer.get());

    void* tex = m_videoModule->GetTexture();
    if (tex) {
        SDL_RenderTexture(m_renderer.get(), static_cast<SDL_Texture*>(tex), nullptr, nullptr);
    }

    m_uiModule->Render(m_videoModule->GetWidth(), m_videoModule->GetHeight(), m_streamUrl, m_running,
                       m_videoModule->IsConnected(), m_hidModule->IsConnected(), m_isCaptured);

    SDL_RenderPresent(m_renderer.get());
}

} // namespace kvm::core
