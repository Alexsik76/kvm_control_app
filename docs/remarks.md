Refactor the architecture of `WebRTCStreamNode.cpp` and `SDLVideoDecoder.cpp` to strictly adhere to SOLID principles, specifically the Single Responsibility Principle (SRP) and Dependency Inversion Principle (DIP).

Required Tasks:
1. Extract the HTTP network logic (`SendWinHttpPost`) from `WebRTCStreamNode.cpp` into a new abstract interface `IHttpClient` (e.g., virtual method `Post(...)`).
2. Create a concrete class `WinHttpClient` that implements `IHttpClient` using the existing WinHTTP API logic. Handle all Windows-specific headers and `#ifdef _WIN32` directives inside this new class only.
3. Apply Dependency Injection: Modify `WebRTCStreamNode` to accept an instance of `IHttpClient` (e.g., via `std::shared_ptr<IHttpClient>`) instead of making direct HTTP calls.
4. Clean up `WebRTCStreamNode.cpp` by removing all direct WinHTTP includes and network utility functions.

Constraints:
- Use standard C++17/C++20 features.
- Provide only the code changes with up to 5 lines of context around the modifications. Do not output the entire files unless explicitly asked.
- Never use the Ukrainian language for code comments in your output.