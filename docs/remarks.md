бачу в цьому файлі дві проблеми: одну синтаксичну і одну архітектурну.

1. Забутий лінк для cpp-httplib
Ти завантажив бібліотеку через FetchContent, але не додав її до target_link_libraries. Додай httplib::httplib:

CMake
 target_link_libraries(${PROJECT_NAME} PRIVATE 
     SDL3::SDL3
     ixwebsocket::ixwebsocket
     nlohmann_json::nlohmann_json
     yaml-cpp
     httplib::httplib
     d3d11.lib
2. Архітектурна пастка з HTTPS (Критично)
Твій Python-бекенд працює через Cloudflare по захищеному протоколу https://pi4.lab.vn.ua/api/....
Бібліотека cpp-httplib здатна робити HTTPS-запити тільки якщо її скомпілювати з OpenSSL. Але ми щойно примусово вимкнули OpenSSL для libdatachannel та ixwebsocket, щоб зменшити розмір бінарників, і перейшли на MbedTLS.

Якщо підключити OpenSSL назад лише заради сигналізації, втрачається весь сенс економії. Можливо, варто відмовитися від cpp-httplib і зробити ці два HTTP-запити (Offer/Answer) через вбудовані у Windows API (WinHTTP або WinINet), які підтримують HTTPS "з коробки" і не потребують зовнішніх бібліотек?