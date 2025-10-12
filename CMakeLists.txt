cmake_minimum_required(VERSION 3.10)
project(NumberPlateRecognition)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск пакетов
find_package(OpenCV REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Concurrent Network)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Создаем исполняемый файл
add_executable(NumberPlateRecognition
    src/NumberPlateRecognizer.h
    src/main.cpp
    src/NumberPlateRecognizer.cpp
    src/main_window.h src/main_window.cpp
    src/async_ocr_client.h src/async_ocr_client.cpp

)

# Линкуем библиотеки
target_link_libraries(NumberPlateRecognition
    ${OpenCV_LIBS}
    Qt5::Core
    Qt5::Widgets
    Qt5::Concurrent
    Qt5::Network
)

target_include_directories(NumberPlateRecognition
    PRIVATE ${OpenCV_INCLUDE_DIRS}
    PRIVATE src
)

# Копируем data файлы
file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
