#include "NumberPlateRecognizer.h"

int main(int argc, char **argv)
{
    NumberPlateRecognizer recognizer;

    // Примеры URL для различных IP-камер:

    // 1. RTSP поток (самый распространенный)
    // std::string url = "rtsp://username:password@192.168.1.100:554/stream1";

    // 2. HTTP/MJPEG поток
    // std::string url = "http://192.168.1.100:8080/video";

    // 3. Пример для тестирования (публичная тестовая камера)
    // std::string url = "rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov";

    std::cout << "IP Camera Number Plate Recognition" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Available URL formats:" << std::endl;
    std::cout << "1. RTSP: rtsp://username:password@ip:port/stream" << std::endl;
    std::cout << "2. HTTP: http://ip:port/video" << std::endl;
    std::cout << "3. MJPEG: http://ip:port/mjpeg" << std::endl;
    std::cout << "==================================" << std::endl;

    std::string url;
    // Проверяем аргументы командной строки
    if (argc > 1) {
        url = argv[1]; // Первый аргумент - URL
        std::cout << "Using URL from command line: " << url << std::endl;
    } else {
        // Если аргументов нет - запрашиваем у пользователя
        std::cout << "Enter camera URL: ";
        std::getline(std::cin, url);

        if (url.empty()) {
            // Default URL если ничего не ввели
            url = "http://127.0.0.1:8080/video";
            std::cout << "Using default URL: " << url << std::endl;
        }
    }

    // Запуск обработки IP-камеры
    recognizer.processIPCamera(url);

    return 0;
}
