# Development Progress: IP-KVM Client

## 1. Current State (Phase 4: IPC Integration & Launcher Support - COMPLETED)

The project has transitioned from a standalone application with static configuration to a managed client integrated with a C# Launcher. This change implements a robust IPC mechanism using **Windows Named Pipes**, allowing for dynamic session management and real-time status reporting.

### Core Achievements (Phase 4):
*   **IPC Integration**: Implemented `ILauncherClient` using Windows Named Pipes for communication with the .NET Launcher.
*   **Dynamic Configuration**: Replaced static `config.yaml` with a **Handshake** protocol. All sensitive data (Access Tokens, Stream URLs, HID URLs) is now securely received from the Launcher at runtime.
*   **Status Reporting**: Added bi-directional messaging, allowing the C++ client to send `StatusUpdate` and `Error` messages back to the Launcher's UI.
*   **Cleanup & Optimization**:
    *   Removed `yaml-cpp` dependency and `AppConfig` module.
    *   Removed redundant network components (`UDPReceiver`, `StubHIDClient`).
    *   Cleaned up documentation and legacy prototype files.
*   **Build System Updates**: Updated `CMakeLists.txt` and `vcpkg.json` to reflect the leaner dependency tree.

### Core Achievements (Previous Phases):
*   **WebRTC Integration**: Uses `libdatachannel` for low-latency video streaming.
*   **RTP Depacketizer**: RFC 6184 compliant depacketizer for H.264 Annex B bitstream generation.
*   **SOLID Architecture**: Strict adherence to SRP, DIP, and RAII principles.
*   **Input Subsystem**: SDL3-based mouse capture (Relative Mode) and scancode translation to Raspberry Pi HID reports.
*   **Overlay GUI**: Integrated **Dear ImGui v1.92.7** for real-time diagnostics and control.

### Updated Architecture (SOLID / SRP / DIP)
*   **WinNamedPipeClient**: Handles low-level Windows Pipe API and JSON messaging with the Launcher.
*   **KVMApplication**: Now accepts `ILauncherClient` to report its lifecycle events (Init, Run, Error).
*   **WebRTCStreamNode**: Manages WebRTC signaling and media decoding.
*   **WinHttpClient**: Used for WebRTC signaling (OFFER/ANSWER exchange).
*   **SDLVideoDecoder**: Renders decoded YUV frames using SDL3.
*   **SDLInputCapturer**: Manages window events and input focus.
*   **WebSocketHIDClient**: Asynchronous HID command transmission via IXWebSocket.

### Technology Stack
*   **Build System**: CMake + MSVC (Visual Studio 2026 / v180).
*   **IPC**: Windows Named Pipes (JSON Protocol).
*   **Dependencies**:
    *   **SDL3** (Windowing/Rendering).
    *   **FFmpeg 8.1** (Decoding).
    *   **libdatachannel** (WebRTC).
    *   **Dear ImGui v1.92.7** (Overlay UI).
    *   **IXWebSocket** (HID Communication).
    *   **nlohmann-json** (IPC & Signaling serialization).
*   **Protocols**: WebRTC (Video), WebSocket (HID), Named Pipes (IPC).

## 2. How to Build and Run

### Build
Use the build script which automatically handles dependencies and MSVC environment:
```powershell
.\build.bat
```

### Run
The client MUST be started with the `--pipe` argument (usually by the Launcher):
```powershell
.\build\Debug\KVMControlApp.exe --pipe <UNIQUE_PIPE_NAME>
```

## 3. Future Roadmap

### Priority 1: Performance & Stability
*   [ ] **Hardware Acceleration**: Explicit DXVA2/D3D11VA integration for H.264 decoding to further reduce CPU usage.
*   [ ] **Reconnection Logic**: Automatic pipe and socket reconnection in case of Launcher or Network failure.
*   [ ] **Clipboard Sync**: Implementation of shared clipboard between local and remote systems.

---
*Last Updated: 07.04.2026 (IPC Integration & Launcher Support Completed)*
