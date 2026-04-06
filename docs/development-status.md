# Development Progress: IP-KVM Client

## 1. Current State (Phase 1, 2 & 3: Video Pipeline, HID Interaction & WebRTC Refactoring - COMPLETED)

A stable, modular architecture for the video pipeline and input subsystem has been implemented. The client uses **SDL3** for modern window management and rendering. The video subsystem has been migrated from raw FFmpeg/UDP to **WebRTC** with a custom RTP depacketizer and a SOLID-compliant network layer.

### Core Achievements (Phase 2 & 3):
*   **WebRTC Integration**: Replaced `FFmpegStreamNode` with `WebRTCStreamNode` using `libdatachannel`.
*   **RTP Depacketizer**: Implemented RFC 6184 compliant depacketizer to handle Single NAL, STAP-A, and FU-A packets, transforming them into Annex B bitstream for FFmpeg.
*   **SOLID Refactoring**: 
    *   Extracted HTTP logic into `IHttpClient` and `WinHttpClient`.
    *   Applied Dependency Injection for network and video modules.
*   **Mouse Capturer**: Implementation of `SDL_SetRelativeMouseMode` (SDL3) for mouse capture.
*   **Event Mapping**: Translation of SDL3 scancodes and modifiers to Raspberry Pi HID reports.
*   **WebSocket Client**: Integrated asynchronous client for HID command transmission with a thread-safe background queue.

### Architecture (SOLID / SRP / DIP)
*   **KVMApplication**: Main orchestrator linking isolated modules (Video, HID, UI, Network) via Dependency Injection.
*   **WebRTCStreamNode**: Isolated node for WebRTC signaling and media reception.
*   **RtpDepacketizer**: Responsible for transforming RTP payloads into a valid H.264 Annex B stream.
*   **WinHttpClient**: Encapsulates Windows-specific WinHTTP API logic for signaling.
*   **SDLVideoDecoder**: Rendering module based on **SDL3**, uploading YUV frames to VRAM.
*   **SDLInputCapturer**: Input capture module via SDL3 (Relative Mouse Mode, Scancodes).
*   **EventMapper**: Responsible for translating SDL Scancodes and modifiers to USB HID codes.
*   **WebSocketHIDClient**: Asynchronous network client based on **IXWebSocket** using a worker thread.
*   **OverlayGUI**: Overlay component based on **Dear ImGui v1.92.7**.

### Technology Stack
*   **Build System**: CMake + MSVC (Visual Studio 2026 / v180).
*   **Dependencies**:
    *   **SDL3** (Video/Events) - Core framework.
    *   **FFmpeg 8.1** (Decoding).
    *   **libdatachannel** (WebRTC).
    *   **Dear ImGui v1.92.7** (GUI).
    *   **IXWebSocket** (Network / WebSocket).
    *   **nlohmann-json** (Data serialization).
*   **Protocols**: WebRTC (Video), WebSocket (HID), HTTPS (Signaling).

## 2. How to Build and Run

### Build
Use the build script which automatically configures the MSVC environment and CMake presets:
```powershell
.\build.bat
```

### Run
The client reads configuration from `config.yaml` or can be extended to accept arguments:
```powershell
.\build\Debug\KVMControlApp.exe
```

## 3. Future Roadmap

### Priority 1: UI/UX Improvements
*   [ ] **Connection Manager**: Startup window to manage a list of KVM nodes.
*   [ ] **Settings**: Configurable hotkeys for releasing capture (e.g., Ctrl+Alt+G).
*   [ ] **Auto-Capture**: Improved logic for seamless cursor transition between local and remote environments.

---
*Last Updated: 06.04.2026 (WebRTC & SOLID Refactoring Completed)*
