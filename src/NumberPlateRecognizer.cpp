#include <NumberPlateRecognizer.h>

#include <QDebug>

NumberPlateRecognizer::NumberPlateRecognizer(QObject *parent)
    : QObject(parent)
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

    // Инициализация переменных для ROI
    roiSelectionMode = false;
    roiSelected = false;
    selectedROI = cv::Rect();

    std::cout << "Number Plate Recognizer initialized successfully!" << std::endl;
}

NumberPlateRecognizer::~NumberPlateRecognizer()
{
    stopProcessing();
    tess.End();
}

void NumberPlateRecognizer::startProcessing(const QString &url)
{
    stopFlag = false;
    processIPCamera(url.toStdString());
}

void NumberPlateRecognizer::stopProcessing()
{
    stopFlag = true;
}

void NumberPlateRecognizer::enableROISelection()
{
    roiSelectionMode = true;
    qDebug() << "ROI selection enabled";
}

void NumberPlateRecognizer::saveROI()
{
    roiSelected = true;
    roiSelectionMode = false;
    qDebug() << "ROI saved:" << selectedROI.width << "x" << selectedROI.height;
    emit roiUpdated(selectedROI.x, selectedROI.y, selectedROI.width, selectedROI.height);
}

void NumberPlateRecognizer::clearROI()
{
    roiSelected = false;
    qDebug() << "ROI cleared";
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

    cv::Mat frame;
    int frameCount = 0;

    // Настройки размера окна
    const std::string WINDOW_NAME = "Number Plate Recognition";
    const int TARGET_WIDTH = 1280;
    const int TARGET_HEIGHT = 720;

    // Создаем resizeable окно
    cv::namedWindow(WINDOW_NAME, cv::WINDOW_NORMAL);
    cv::resizeWindow(WINDOW_NAME, TARGET_WIDTH, TARGET_HEIGHT);
    cv::setMouseCallback(WINDOW_NAME, onMouse, this);

    while (!stopFlag) {
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Failed to grab frame from IP camera" << std::endl;
            break;
        }

        frameCount++;

        // Создаем копию для отрисовки
        cv::Mat displayFrame = frame.clone();
        // Получаем текущий ROI (если не выбран - используем весь кадр)
        cv::Rect currentROI = roiSelected ? selectedROI : cv::Rect(0, 0, frame.cols, frame.rows);
        // Рисуем интерфейс в зависимости от режима
        if (roiSelectionMode) {
            // Режим выбора ROI - только показываем видео и рисуем прямоугольник
            if (drawing || (selectedROI.width > 0 && selectedROI.height > 0)) {
                cv::rectangle(displayFrame, selectedROI, cv::Scalar(0, 255, 255), 2);

                std::string rectInfo = "Selection: " + std::to_string(selectedROI.x) + ","
                    + std::to_string(selectedROI.y) + " " + std::to_string(selectedROI.width) + "x"
                    + std::to_string(selectedROI.height);
                cv::putText(displayFrame, rectInfo, cv::Point(selectedROI.x, selectedROI.y - 10),
                            cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 255), 2);
            }

            // Инструкции для режима выбора ROI
            cv::putText(displayFrame, "ROI SELECTION MODE - Draw rectangle with mouse",
                        cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255),
                        2);
            cv::putText(displayFrame, "Press '2' to save, '3' to cancel", cv::Point(10, 60),
                        cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
        } else {
            // Обрабатываем каждый 3-й кадр для скорости
            if (frameCount % 3 == 0) {
                // Распознаем только в выбранной области (или во всем кадре)
                cv::Mat processingArea = frame(currentROI);

                std::vector<cv::Rect> plates = detectPlates(processingArea);

                // Корректируем координаты если используется ROI
                if (roiSelected && !plates.empty()) {
                    for (auto &plate : plates) {
                        plate.x += currentROI.x;
                        plate.y += currentROI.y;
                    }
                }

                detectText(frame, plates);
            }

            // Статусная информация
            std::string roiStatus = roiSelected ? "ROI: " + std::to_string(currentROI.width) + "x"
                    + std::to_string(currentROI.height)
                                                : "FULL FRAME";

            cv::putText(displayFrame, "Mode: " + roiStatus, cv::Point(10, 30),
                        cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 0), 2);
        }

        // Общая информация
        double fps = cap.get(cv::CAP_PROP_FPS);
        cv::putText(displayFrame, "FPS: " + std::to_string((int)fps),
                    cv::Point(10, frame.rows - 80), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                    cv::Scalar(0, 255, 255), 1);

        // Инструкции
        cv::putText(displayFrame,
                    "Press '1': Select ROI | '2': Save ROI | '3': Clear ROI | 'q': Quit",
                    cv::Point(10, frame.rows - 10), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                    cv::Scalar(255, 255, 0), 1);
        // Показываем результат
        cv::imshow(WINDOW_NAME, displayFrame);

        cv::waitKey(1);
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

void NumberPlateRecognizer::handleMouse(int event, int x, int y, int flags)
{
    // Обрабатываем события мыши только в режиме выбора ROI
    if (!roiSelectionMode)
        return;

    if (event == cv::EVENT_LBUTTONDOWN) {
        // Начало рисования
        drawing = true;
        startPoint = cv::Point(x, y);
        endPoint = startPoint;
        selectedROI = cv::Rect();
    } else if (event == cv::EVENT_MOUSEMOVE && drawing) {
        // Обновление прямоугольника при движении мыши
        endPoint = cv::Point(x, y);
        selectedROI = cv::Rect(startPoint, endPoint);
    } else if (event == cv::EVENT_LBUTTONUP) {
        // Завершение рисования
        drawing = false;
        endPoint = cv::Point(x, y);

        // Создаем правильный прямоугольник (могут быть отрицательные размеры)
        int x1 = std::min(startPoint.x, endPoint.x);
        int y1 = std::min(startPoint.y, endPoint.y);
        int x2 = std::max(startPoint.x, endPoint.x);
        int y2 = std::max(startPoint.y, endPoint.y);

        selectedROI = cv::Rect(x1, y1, x2 - x1, y2 - y1);

        std::cout << "Rectangle drawn: " << selectedROI.x << ", " << selectedROI.y << " "
                  << selectedROI.width << "x" << selectedROI.height << std::endl;
    }
}

void NumberPlateRecognizer::onMouse(int event, int x, int y, int flags, void *userdata)
{
    NumberPlateRecognizer *recognizer = static_cast<NumberPlateRecognizer *>(userdata);
    recognizer->handleMouse(event, x, y, flags);
}
