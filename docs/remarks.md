To avoid manual token copying, the application must automatically fetch a JWT access token from the backend API on startup.

Task: Implement a machine-to-machine authentication step before establishing the WebRTC and WebSocket connections (e.g., inside `KVMApplication::Start` or similar initialization method).

1. Read authentication parameters from `config.yaml` (e.g., `auth.login_url`, `auth.username`, and `auth.password`).
2. Use the existing `IHttpClient` to send a POST request to `login_url`. Send the credentials as a JSON payload: `{"username": "<user>", "password": "<pass>"}` (or `application/x-www-form-urlencoded` if standard for your backend).
3. Parse the JSON response to extract the `access_token` string.
4. Dynamically append `?token=<access_token>` to the WebRTC signaling URL and WebSocket HID URL.
5. If the token fetch fails, log an error to `std::cerr` and halt initialization.