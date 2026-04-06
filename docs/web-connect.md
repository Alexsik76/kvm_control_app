Role: You are a Senior C++ Developer. We are continuing the development of our native IP-KVM client.

Task: Implement the entry-point logic to support a Custom URI Scheme (Deep Linking). The native application will be launched by the OS via a web browser click, receiving the connection parameters as a command-line argument.

Input Data:
The OS will pass a URI in argv[1] following this format:
ipkvm://connect?host=wss://kvm1.domain.com:8443/ws/control&token=eyJhbG...

Architectural Requirements (SOLID, SRP):

UriParser Module: Create an isolated class/utility responsible strictly for parsing the URI string. It should extract the host and token values.

Data Structure: Use a dedicated struct (e.g., ConnectionParams) to return the parsed data.

Robustness: Handle edge cases gracefully (e.g., empty arguments, malformed URIs, missing query parameters). Do not crash; throw a descriptive C++ exception or return an error type (e.g., std::optional or std::expected from C++23 if available, otherwise std::optional in C++20).

Modern C++: Use std::string_view for zero-allocation string parsing where appropriate.

Code Quality Standards:

Comments: All comments must be in English only.

Memory: Strict RAII.

Incremental Delivery: Do not write OS-specific installer scripts (like Windows Registry or macOS Info.plist) right now. Focus only on the C++ parsing logic and the main() function integration.

Instruction for this iteration:

Generate the ConnectionParams struct and the UriParser header/implementation.

Provide a snippet of the int main(int argc, char* argv[]) function showing how the parsed data is passed to the IHIDClient::connect() method we defined earlier.
