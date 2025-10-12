#include <NumberPlateRecognizer.h>

#include <QDebug>

NumberPlateRecognizer::NumberPlateRecognizer(QObject *parent)
    : QObject(parent)
{
    std::cout << "Initializing Number Plate Recognizer..." << std::endl;

    // Загрузка каскада для детекции номеров
    std::string cascadePath = "haarcascade_russian_plate_number.xml";
    if (!plateCascade.load(cascadePath)) {
        std::cerr << "Could not load plate cascade from: " << cascadePath << std::endl;
        std::cerr << "Using contour-based detection only" << std::endl;
    }

    // Инициализация переменных для ROI
    roiSelectionMode = false;
    roiSelected = false;
    selectedROI = cv::Rect();

    // Инициализация OCR клиента
    ocrClient = new AsyncOCRClient(this);
    connect(ocrClient, &AsyncOCRClient::plateRecognized, this,
            &NumberPlateRecognizer::onOCRResultReceived);

    std::cout << "Number Plate Recognizer initialized successfully!" << std::endl;
}

NumberPlateRecognizer::~NumberPlateRecognizer()
{
    stopProcessing();
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
    cv::Mat image2;
    cv::cvtColor(image, image2, cv::COLOR_RGB2GRAY);

    cv::Mat image3;
    cv::medianBlur(image2, image3, 3);

    return image3;
}

// Детекция области с номером
cv::Mat NumberPlateRecognizer::detectPlate(const cv::Mat &image)
{
    cv::Mat image2;
    cv::cvtColor(image, image2, cv::COLOR_BGR2RGB);

    // Детекция с каскадом Хаара
    std::vector<cv::Rect> plates;
    plateCascade.detectMultiScale(image2, plates, 1.1, 10, 0);
    if (plates.size()) {
        std::cout << "Haar cascade found: " << plates.size() << " regions" << std::endl;
    }

    if (plates.empty()) {
        return {};
    }

    cv::Mat image3 = image2(plates[0]);
    return upscalePlateSimple(image3);
}

void NumberPlateRecognizer::detectText(const cv::Mat &frame)
{
    // Распознавание
    cv::Mat plateROI = frame;
    std::string plateText = recognizeText(plateROI);
    if (!plateText.empty()) {
        std::cout << "=== Detected plate: " << plateText << " ===" << std::endl;
    }
}

// Распознавание текста с номера
std::string NumberPlateRecognizer::recognizeText(const cv::Mat &plateImage)
{
    // Увеличиваем область для лучшего распознавания
    cv::Mat processed = preprocessImage(plateImage);
    Q_UNUSED(processed);

    // Получение текста
    std::string result;

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
        } else {
            // Обычный режим - отправляем кадры на распознавание
            if (!isOCRProcessing) {
                isOCRProcessing = true;

                // Берем ROI область для распознавания
                cv::Mat roiArea = frame(currentROI);
                cv::Mat roiPlate = detectPlate(roiArea);

                // Отправляем на распознавание (асинхронно)
                ocrClient->submitFrameForRecognition(roiPlate);
            }
        }

        // Показываем результат
        cv::imshow(WINDOW_NAME, displayFrame);

        cv::waitKey(1);
    }

    cap.release();
    cv::destroyAllWindows();
    isOCRProcessing = false;

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

cv::Mat NumberPlateRecognizer::upscalePlateSimple(const cv::Mat &plate_image, int scale)
{
    // Проверка на пустое изображение
    if (plate_image.empty()) {
        return cv::Mat();
    }

    int height = plate_image.rows;
    int width = plate_image.cols;
    int new_width = width * scale;
    int new_height = height * scale;

    cv::Mat upscaled;
    cv::resize(plate_image, upscaled, cv::Size(new_width, new_height), 0, 0, cv::INTER_CUBIC);

    cv::Mat kernel = (cv::Mat_<float>(3, 3) << -1, -1, -1, -1, 9, -1, -1, -1, -1);

    cv::Mat sharpened;
    cv::filter2D(upscaled, sharpened, -1, kernel);

    return sharpened;
}

void NumberPlateRecognizer::onOCRResultReceived(const QString &plateText, double confidence)
{
    isOCRProcessing = false;

    if (!plateText.isEmpty()) {
        std::cout << "=== Detected plate: " << plateText.toStdString()
                  << " (confidence: " << confidence << ") ===" << std::endl;
    }

    // Можно также emit-ить сигнал для MainWindow
    emit plateDetected(plateText, confidence);
}
