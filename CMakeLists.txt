cmake_minimum_required(VERSION 3.10)
project(NumberPlateRecognition)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск пакетов
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

pkg_check_modules(TESSERACT REQUIRED tesseract)
pkg_check_modules(LEPTONICA REQUIRED lept)

# Включаем директории
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${TESSERACT_INCLUDE_DIRS})
include_directories(${LEPTONICA_INCLUDE_DIRS})
include_directories(src)

qt6_standard_project_setup()

# Создаем исполняемый файл
add_executable(NumberPlateRecognition
    src/NumberPlateRecognizer.h
    src/main.cpp
    src/NumberPlateRecognizer.cpp
    src/main_window.h src/main_window.cpp
)

# Линкуем библиотеки
target_link_libraries(NumberPlateRecognition
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
    ${OpenCV_LIBS}
    ${TESSERACT_LIBRARIES}
    ${LEPTONICA_LIBRARIES}
)

# Копируем data файлы
file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
