#include <NumberPlateRecognizer.h>

NumberPlateRecognizer::NumberPlateRecognizer()
{
    std::cout << "Initializing Number Plate Recognizer..." << std::endl;

    // Инициализация Tesseract
    if (tess.Init(NULL, "rus+eng", tesseract::OEM_LSTM_ONLY)) {
        std::cerr << "Could not initialize tesseract!" << std::endl;
        exit(1);
    }

    // Настройка Tesseract
    tess.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
    tess.SetVariable("tessedit_char_whitelist", "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

    // Загрузка каскада для детекции номеров
    std::string cascadePath = "data/haarcascade_russian_plate_number.xml";
    if (!plateCascade.load(cascadePath)) {
        std::cerr << "Could not load plate cascade from: " << cascadePath << std::endl;
        std::cerr << "Using contour-based detection only" << std::endl;
    }

    std::cout << "Number Plate Recognizer initialized successfully!" << std::endl;
}

NumberPlateRecognizer::~NumberPlateRecognizer()
{
    tess.End();
}

// Предобработка изображения
cv::Mat NumberPlateRecognizer::preprocessImage(const cv::Mat &image)
{
    cv::Mat gray, blurred, thresholded;

    // Конвертация в grayscale
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    // Увеличение резкости
    cv::Mat kernel = (cv::Mat_<float>(3, 3) << -1, -1, -1, -1, 9, -1, -1, -1, -1);
    cv::filter2D(gray, gray, -1, kernel);

    // Билатеральная фильтрация для сохранения границ
    cv::bilateralFilter(gray, blurred, 11, 17, 17);

    // Адаптивная thresholding
    cv::adaptiveThreshold(blurred, thresholded, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY, 11, 2);

    return thresholded;
}

// Детекция области с номером
std::vector<cv::Rect> NumberPlateRecognizer::detectPlates(const cv::Mat &image)
{
    std::vector<cv::Rect> plates;
    cv::Mat gray;

    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(gray, gray);

    // Детекция с каскадом Хаара
    plateCascade.detectMultiScale(gray, plates, 1.1, 10, 0);
    if (plates.size()) {
        std::cout << "Haar cascade found: " << plates.size() << " regions" << std::endl;
    }

    return plates;
}

void NumberPlateRecognizer::detectText(const cv::Mat &frame, const std::vector<cv::Rect> &plates)
{
    for (size_t i = 0; i < plates.size(); i++) {
        const auto &plateRect = plates[i];
        // Вырезаем область номера
        cv::rectangle(frame, plateRect, {255, 0, 0}, 5);

        // Распознавание
        cv::Mat plateROI = frame(plateRect);
        std::string plateText = recognizeText(plateROI);

        if (!plateText.empty()) {
            // Рисуем bounding box
            cv::rectangle(frame, plateRect, cv::Scalar(0, 255, 0), 3);
            // Добавляем текст
            cv::putText(frame, plateText, cv::Point(plateRect.x, plateRect.y - 10),
                        cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);
            std::cout << "=== Detected plate: " << plateText << " ===" << std::endl;
        }
    }
}

// Распознавание текста с номера
std::string NumberPlateRecognizer::recognizeText(const cv::Mat &plateImage)
{
    // Увеличиваем область для лучшего распознавания
    cv::Mat processed = preprocessImage(plateImage);

    // Установка изображения для Tesseract
    tess.SetImage(processed.data, processed.cols, processed.rows, processed.elemSize(),
                  processed.step);

    // Получение текста
    char *text = tess.GetUTF8Text();
    std::string result(text ? text : "");
    if (text) {
        delete[] text;
    }

    std::cout << "OCR result: '" << result << "'" << std::endl;

    // Очистка текста
    return result;
}

// Очистка и валидация номера
std::string NumberPlateRecognizer::cleanNumberPlateText(const std::string &text)
{
    // Удаление лишних символов
    std::string cleaned;
    for (char c : text) {
        if (std::isalnum(c)) {
            cleaned += std::toupper(c);
        }
    }

    // Простая валидация по длине
    if (cleaned.length() >= 6 && cleaned.length() <= 9) {
        std::cout << "Recognized: " << cleaned << std::endl;
        return cleaned;
    }

    return "";
}

// Обработка IP-камеры
void NumberPlateRecognizer::processIPCamera(const std::string &url)
{
    std::cout << "Connecting to IP camera: " << url << std::endl;

    cv::VideoCapture cap(url);
    if (!cap.isOpened()) {
        std::cerr << "Error opening IP camera: " << url << std::endl;
        return;
    }

    std::cout << "Successfully connected to IP camera!" << std::endl;
    std::cout << "Press 'q' to quit, 's' to save current frame" << std::endl;

    cv::Mat frame;
    int frameCount = 0;

    while (true) {
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Failed to grab frame from IP camera" << std::endl;
            break;
        }

        frameCount++;

        // Обрабатываем каждый 5-й кадр для скорости
        if (frameCount % 3 == 0) {
            // Поиск plate-ов с ГРЗ на изображении
            std::vector<cv::Rect> plates = detectPlates(frame);
            // Отладочная информация для каждого найденного региона
            for (size_t i = 0; i < plates.size(); i++) {
                std::cout << "Plate " << i << ": " << plates[i].x << "," << plates[i].y << " "
                          << plates[i].width << "x" << plates[i].height << std::endl;
            }

            // Распознаем текст на plate
            detectText(frame, plates);
        }

        // Показываем FPS
        double fps = cap.get(cv::CAP_PROP_FPS);
        cv::putText(frame, "FPS: " + std::to_string((int)fps), cv::Point(10, 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);

        // Показываем источник
        cv::putText(frame, "Source: " + url, cv::Point(10, frame.rows - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);

        // Показываем результат
        cv::imshow("Number Plate Recognition - IP Camera", frame);

        // Обработка клавиш
        int key = cv::waitKey(1);
        if (key == 'q' || key == 27) { // 'q' или ESC
            break;
        } else if (key == 's') { // Сохранение кадра
            std::string filename = "frame_" + std::to_string(frameCount) + ".jpg";
            cv::imwrite(filename, frame);
            std::cout << "Frame saved as: " << filename << std::endl;
        }
    }

    cap.release();
    cv::destroyAllWindows();

    // Вывод итогов
    std::cout << "\n=== Detection Summary ===" << std::endl;
}

// Специальные методы для разных протоколов
void NumberPlateRecognizer::processRTSP(const std::string &rtspUrl)
{
    std::cout << "Connecting to RTSP stream..." << std::endl;
    processIPCamera(rtspUrl);
}

void NumberPlateRecognizer::processHTTP(const std::string &httpUrl)
{
    std::cout << "Connecting to HTTP stream..." << std::endl;
    processIPCamera(httpUrl);
}
