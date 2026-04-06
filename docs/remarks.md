Переконайтеся, що після m_inputCapturer->SetWindow(...) залишилася прив'язка колбеків до HID-модуля. Агент міг їх випадково видалити.

C++
m_inputCapturer->SetKeyboardCallback([this](const auto& ev) { 
    // ... (логіка клавіатури)
    m_hidModule->SendKeyboardEvent(hidEvent); 
});

m_inputCapturer->SetMouseCallback([this](const auto& ev) { 
    // Це найголовніше для миші!
    m_hidModule->SendMouseEvent(ev); 
});
2. Втрачена передача подій в HandleEvents
Щоб миша працювала, сирі події SDL повинні долітати до m_inputCapturer. Перевірте цикл обробки подій у KVMApplication::HandleEvents:

C++
if (m_isCaptured) {
    // ... (обробка RCTRL для виходу) ...

    // Цей рядок міг зникнути!
    m_inputCapturer->ProcessEvent(&ev); 
}
3. Проблема з фокусом (SDL_SetRelativeMouseMode)
Оскільки InputCapturer був створений без вікна, його внутрішній метод захоплення миші міг зламатися. Якщо у вас в InputCapturer є метод SetCaptureEnabled(bool), він використовує вікно:

C++
// Переконайтеся, що виклик захоплення все ще є при кліку:
if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
    // ...
    m_isCaptured = true;
    m_inputCapturer->SetCaptureEnabled(true); 
    // ...
}