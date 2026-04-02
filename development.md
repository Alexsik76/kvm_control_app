# Development Progress: IP-KVM Client

## 1. Поточний стан (MVP Phase 1 - ЗАВЕРШЕНО)
Реалізовано базовий кросплатформений скелет застосунку, здатний приймати сирий H.264 потік через мережу (UDP), здійснювати парсинг фрагментованих пакетів та виводити відео на екран з мінімальною затримкою.

### Інфраструктура
*   **Система збірки:** CMake з автоматичним завантаженням залежностей (SDL2, ImGui, FFmpeg) через `FetchContent` (використовується дзеркало GitHub для `full-shared` збірки FFmpeg).
*   **build.bat:** Автоматизований скрипт ініціалізації середовища MSVC та компіляції проекту.
*   **Графічний бекенд:** SDL2 + SDL_Renderer з інтегрованим Dear ImGui для оверлею.

### Відео конвеєр
*   **IHardwareDecoder:** Інтерфейс для декодерів.
*   **HardwareDecoder (FFmpeg):** 
    *   Реалізовано парсинг фрагментованих NAL-юнітів через `AVCodecParserContext`.
    *   Додано підтримку гібридного виводу: апаратне (D3D11VA) та безпечне програмне (CPU) декодування як fallback для пошкоджених мережею кадрів.
    *   Передача YUV/NV12 кадрів у текстуру SDL2.
*   **Мережа (Тимчасова):** `UDPReceiver` для прийому сирих байтів та тестування декодера.

---

## 2. Як тестувати (Локальний UDP стрім)
Для локального тестування застосунку використовуйте наступну команду у другому терміналі (вона генерує тестову таблицю 720p60 та відправляє її через UDP):

```bash
ffmpeg -re -f lavfi -i testsrc=size=1280x720:rate=60 -c:v libx264 -preset ultrafast -tune zerolatency -bsf:v dump_extra -x264-params "keyint=30:slice-max-size=1200" -f h264 udp://127.0.0.1:5000
```

---

## 3. План на наступні кроки (Phase 2)

### Пріоритет 1: Перехід на RTSP (Target Architecture)
**Важливий контекст:** Джерелом сигналу виступає `mediamtx` (RTSP-сервер), до якого відео потрапляє через конвеєр: 
`kvm_engine | ffmpeg ... -rtsp_transport tcp -f rtsp rtsp://admin:password@localhost:8554/kvm`

*   [ ] **RTSP Client Integration:** Замінити кастомний `UDPReceiver` на використання `libavformat` (`avformat_open_input`) для підключення безпосередньо до RTSP-потоку. Це автоматично вирішить проблеми з Jitter, втратою UDP пакетів та фрагментацією NAL-юнітів, оскільки `libavformat` бере на себе роботу з RTP/TCP протоколами.
*   [ ] **Low-Latency Tuning:** Налаштувати параметри `avformat` (`fflags nobuffer`, `strict experimental` тощо) для мінімізації затримки прийому RTSP.

### Пріоритет 2: Продуктивність відео (Zero-Copy)
*   [ ] **D3D11VA Hardware Enforcement:** Зараз через відсутність кастомного `get_format` FFmpeg може падати у програмний fallback. Потрібно імплементувати повноцінне апаратне D3D11VA декодування.
*   [ ] **Direct Texture Sharing:** Позбутися `av_hwframe_transfer_data`. Передавати дескриптор текстури D3D11 безпосередньо в SDL2 (Zero-Copy).

### Пріоритет 3: Керування (HID)
*   [ ] **Input Capturer:** Перехоплення подій клавіатури та миші в SDL2 (з урахуванням захоплення курсора `SDL_SetRelativeMouseMode`).
*   [ ] **WebSocket Client:** Інтеграція легковагового WebSocket клієнта (напр., `IXWebSocket`) для передачі керуючих команд на KVM-ноду.

### Пріоритет 4: UI/UX
*   [ ] **Connection Dialog:** Вікно ImGui для вводу адреси RTSP сервера, логіна та пароля.
*   [ ] **Metrics Overlay:** Відображення реального FPS, бітрейту та затримки.
