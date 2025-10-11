#include "NumberPlateRecognizer.h"
#include "main_window.h"
#include <QApplication>

int main(int argc, char *argv[])
{
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

    QString url;
    // Проверяем аргументы командной строки
    if (argc < 2) {
        // Если аргументов нет
        std::cout << "Enter camera URL";
        exit(1);
    }

    url = argv[1]; // Первый аргумент - URL
    std::cout << "Using URL from command line: " << url.toStdString() << std::endl;

    QApplication app(argc, argv);

    MainWindow window(url);
    window.show();

    return app.exec();
}

#if 0
int main(int argc, char **argv)
{
    NumberPlateRecognizer recognizer;



    // Запуск обработки IP-камеры
    recognizer.processIPCamera(url);

    return 0;
}
#endif
