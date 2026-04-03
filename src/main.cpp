#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "video/IVideoDecoder.hpp"
#include "ui/OverlayGUI.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    SDL_SetMainReady();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "[SDL] Init error: " << SDL_GetError() << "\n";
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "IP-KVM Client",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "[SDL] Renderer error: " << SDL_GetError() << "\n";
        return -1;
    }

    auto gui = std::make_unique<kvm::ui::OverlayGUI>(window, renderer);

    auto decoder = kvm::video::CreateVideoDecoder(renderer);
    if (!decoder->Initialize()) {
        std::cerr << "[Decoder] Initialization failed\n";
        return -1;
    }

    std::string url = "udp://127.0.0.1:5000";
    if (argc > 1) {
        url = argv[1];
    }
    decoder->OpenStream(url);

    bool done = false;
    while (!done) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            gui->ProcessEvent(&ev);
            if (ev.type == SDL_QUIT) done = true;
        }

        gui->NewFrame();

        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);

        void* tex = decoder->GetTexture();
        if (tex) {
            SDL_RenderCopy(renderer, static_cast<SDL_Texture*>(tex), nullptr, nullptr);
        }

        gui->Render(decoder->GetWidth(), decoder->GetHeight(), url, done);

        SDL_RenderPresent(renderer);
    }

    return 0;
}
