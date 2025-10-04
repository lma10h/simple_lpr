cmake_minimum_required(VERSION 3.10)
project(NumberPlateRecognition)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск пакетов
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(TESSERACT REQUIRED tesseract)
pkg_check_modules(LEPTONICA REQUIRED lept)

# Включаем директории
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${TESSERACT_INCLUDE_DIRS})
include_directories(${LEPTONICA_INCLUDE_DIRS})
include_directories(src)

# Создаем исполняемый файл
add_executable(plate_recognition 
    src/NumberPlateRecognizer.h
    src/main.cpp
    src/NumberPlateRecognizer.cpp
)

# Линкуем библиотеки
target_link_libraries(plate_recognition 
    ${OpenCV_LIBS}
    ${TESSERACT_LIBRARIES}
    ${LEPTONICA_LIBRARIES}
)

# Копируем data файлы
file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
