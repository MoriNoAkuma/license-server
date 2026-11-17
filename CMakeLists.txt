cmake_minimum_required(VERSION 3.21)
project(LicenseServer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
qt_standard_project_setup()
set(CMAKE_AUTORCC ON) # resources.qrc: шрифти, іконки, style.qss

# ядро без GUI: модель, сховище, мережевий сервер - переживе заміну інтерфейсу
qt_add_library(licensecore STATIC
    server/license.h server/license.cpp
    server/licensestore.h server/licensestore.cpp
    server/licenseserver.h server/licenseserver.cpp
)
target_include_directories(licensecore PUBLIC server)
target_link_libraries(licensecore PUBLIC Qt6::Core Qt6::Network)

# модуль, який підключається в захищену програму
qt_add_library(licenseclient STATIC client/licenseclient.h client/licenseclient.cpp)
target_include_directories(licenseclient PUBLIC client)
target_link_libraries(licenseclient PUBLIC Qt6::Widgets Qt6::Network)

# панель адміністратора
qt_add_executable(LicenseServer WIN32
    server/main.cpp
    server/mainwindow.h server/mainwindow.cpp
    server/licensedialog.h server/licensedialog.cpp
    server/resources.qrc
)
target_link_libraries(LicenseServer PRIVATE licensecore Qt6::Widgets)

enable_testing()
qt_add_executable(tests tests/tests.cpp)
target_link_libraries(tests PRIVATE licensecore licenseclient)
target_compile_options(tests PRIVATE -UNDEBUG) # тести на assert мають працювати і в Release
add_test(NAME tests COMMAND tests)
