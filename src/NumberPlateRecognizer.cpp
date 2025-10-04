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
    if (!plateCascade.empty()) {
        plateCascade.detectMultiScale(gray, plates, 1.1, 3, 0, cv::Size(80, 30),
                                      cv::Size(300, 100));
    }

    // Если каскад не нашел, используем контуры
    if (plates.empty()) {
        plates = findPlatesByContours(image);
    }

    return plates;
}

// Альтернативный метод через контуры
std::vector<cv::Rect> NumberPlateRecognizer::findPlatesByContours(const cv::Mat &image)
{
    std::vector<cv::Rect> plates;
    cv::Mat processed = preprocessImage(image);

    // Морфологические операции для улучшения контуров
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(processed, processed, cv::MORPH_CLOSE, kernel);

    // Поиск контуров
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(processed, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // Фильтрация контуров по размеру и форме
    for (const auto &contour : contours) {
        cv::Rect rect = cv::boundingRect(contour);
        double aspectRatio = (double)rect.width / rect.height;
        double area = cv::contourArea(contour);
        double extent = area / (rect.width * rect.height);

        // Типичные пропорции номерного знака
        if (rect.width > 100 && rect.height > 30 && rect.width < 500 && rect.height < 150
            && aspectRatio > 2.0 && aspectRatio < 5.0 && extent > 0.4) {
            plates.push_back(rect);
        }
    }

    return plates;
}

// Распознавание текста с номера
std::string NumberPlateRecognizer::recognizeText(const cv::Mat &plateImage)
{
    // Увеличиваем область для лучшего распознавания
    cv::Mat enlarged;
    cv::resize(plateImage, enlarged, cv::Size(plateImage.cols * 2, plateImage.rows * 2), 0, 0,
               cv::INTER_CUBIC);

    cv::Mat processed = preprocessImage(enlarged);

    // Установка изображения для Tesseract
    tess.SetImage(processed.data, processed.cols, processed.rows, 1, processed.step);

    // Получение текста
    char *text = tess.GetUTF8Text();
    std::string result(text ? text : "");
    if (text)
        delete[] text;

    // Очистка текста
    return cleanNumberPlateText(result);
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
    std::vector<std::string> detectedPlates;

    while (true) {
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Failed to grab frame from IP camera" << std::endl;
            break;
        }

        frameCount++;

        // Обрабатываем каждый 5-й кадр для скорости
        if (frameCount % 5 == 0) {
            // Детекция номеров
            std::vector<cv::Rect> plates = detectPlates(frame);

            for (const auto &plateRect : plates) {
                // Вырезаем область номера
                cv::Mat plateROI = frame(plateRect);

                // Распознаем текст
                std::string plateText = recognizeText(plateROI);

                if (!plateText.empty()) {
                    // Проверяем дубликаты
                    bool isDuplicate = false;
                    for (const auto &existing : detectedPlates) {
                        if (existing == plateText) {
                            isDuplicate = true;
                            break;
                        }
                    }

                    if (!isDuplicate) {
                        detectedPlates.push_back(plateText);

                        // Рисуем bounding box
                        cv::rectangle(frame, plateRect, cv::Scalar(0, 255, 0), 3);

                        // Добавляем текст
                        cv::putText(frame, plateText, cv::Point(plateRect.x, plateRect.y - 10),
                                    cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

                        std::cout << "=== Detected plate: " << plateText << " ===" << std::endl;
                    }
                }
            }
        }

        // Показываем FPS
        double fps = cap.get(cv::CAP_PROP_FPS);
        cv::putText(frame, "FPS: " + std::to_string((int)fps), cv::Point(10, 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);

        // Показываем количество обнаруженных номеров
        cv::putText(frame, "Plates detected: " + std::to_string(detectedPlates.size()),
                    cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);

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
    std::cout << "Total plates detected: " << detectedPlates.size() << std::endl;
    for (size_t i = 0; i < detectedPlates.size(); i++) {
        std::cout << i + 1 << ": " << detectedPlates[i] << std::endl;
    }
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
