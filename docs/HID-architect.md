# HID architect

Role: You are a Senior C++ Developer and System Architect. Your task is to design and implement a high-performance, cross-platform (Windows, Linux, macOS) HID (Keyboard/Mouse) client for an IP-KVM system.

System Overview:
The client captures raw keyboard and mouse inputs and sends them to a remote hardware node via WebSockets with ultra-low latency. (Note: Video streaming is handled by a separate process/module, focus strictly on HID).

Technology Stack:

Language: C++20.

Input Capture: SDL3 (using RelativeMouseMode and Scancodes for raw hardware input).

Networking: WebSockets (e.g., IXWebSocket) for asynchronous JSON payload transmission.

Architectural Requirements (SOLID, SRP, RAII):
The application must be modular:

InputCapturer: Captures raw events using SDL3, bypassing OS-level acceleration and keyboard layouts.

EventMapper: Translates SDL3 events into standard USB HID Usage Tables (scan-codes and bitmasks).

HIDClient: An asynchronous WebSocket client that sends JSON-encoded HID reports to the hardware node.

Code Quality Standards:

Comments: All comments must be in English only.

Self-Documenting Code: Use intention-revealing names.

Memory Management: Strictly follow RAII (std::unique_ptr, std::shared_ptr, STL containers). No manual new/delete.

No Dead Code: Do not leave commented-out code blocks.

Instruction for the first iteration:

Propose a directory structure for this standalone HID client.

Define the base abstract interfaces (pure virtual classes) for the InputCapturer and HIDClient modules.

Keep the code clean and demonstrate loose coupling. Do not generate the full implementation yet.

## base classes example

```c++
#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <string>

// Represents a standard USB HID keyboard report
struct KeyboardEvent {
    uint8_t modifiers{0};
    std::vector<uint8_t> scanCodes;
};

// Represents a standard USB HID mouse report
struct MouseEvent {
    uint8_t buttons{0};
    int16_t deltaX{0};
    int16_t deltaY{0};
    int8_t wheel{0};
};

// Interface for capturing raw hardware input
class IInputCapturer {
public:
    virtual ~IInputCapturer() = default;

    // Injects callbacks to decouple input capturing from network transmission
    virtual void setKeyboardCallback(std::function<void(const KeyboardEvent&)> callback) = 0;
    virtual void setMouseCallback(std::function<void(const MouseEvent&)> callback) = 0;
    
    // Non-blocking poll for hardware events
    virtual void pollEvents() = 0;
};

// Interface for asynchronous network transmission
class IHIDClient {
public:
    virtual ~IHIDClient() = default;

    // Establishes connection to the KVM node
    virtual bool connect(const std::string& url) = 0;
    
    // Thread-safe methods to enqueue and send events
    virtual void sendKeyboardEvent(const KeyboardEvent& event) = 0;
    virtual void sendMouseEvent(const MouseEvent& event) = 0;
};
```
