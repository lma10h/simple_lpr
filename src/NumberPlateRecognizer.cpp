#include <NumberPlateRecognizer.h>

#include <QDebug>

#include <algorithm>
#include <cmath>

void nothing()
{
    /*cv::Mat roiPlate = detectPlate(roiArea);
    if (!roiPlate.empty()) {
        cv::imwrite("frames/frame_" + std::to_string(frameNum) + "_roiPlate.jpg", roiPlate);

        isOCRProcessing = true;

        // Коррекция перекоса
        auto [angle, alignedPlate] = correct_skew(roiPlate, 1, 15);
        std::cout << "Skew angle:" << angle << "\n";

        // preprocessImage
        cv::cvtColor(alignedPlate, alignedPlate, cv::COLOR_BGR2GRAY);
                        cv::medianBlur(alignedPlate, alignedPlate, 5);
                        // CLAHE - адаптивное выравнивание гистограммы
                        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
                        clahe->setClipLimit(3.0);
                        clahe->setTilesGridSize(cv::Size(8, 8));

                        cv::Mat enhanced;
                        clahe->apply(alignedPlate, enhanced);

        cv::imwrite("frames/frame_" + std::to_string(frameNum) + "_roiPlate_Aligned.jpg",
                    alignedPlate);
        ocrClient->submitFrameForRecognition(alignedPlate);
    }*/
}

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

// Детекция области с номером
cv::Mat NumberPlateRecognizer::detectPlate(cv::Mat image)
{
    // Детекция с каскадом Хаара
    std::vector<cv::Rect> plates;

    plateCascade.detectMultiScale(image, plates, 1.1, 10, 0);
    if (plates.empty()) {
        return {};
    }

    return image(plates[0]);
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

    // Таймер для ограничения частоты запросов
    auto last_ocr_time = std::chrono::steady_clock::now();
    const std::chrono::milliseconds ocr_interval(300); // 1 секунда

    int frameNum = 0;
    while (!stopFlag) {
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Failed to grab frame from IP camera" << std::endl;
            break;
        }

        ++frameNum;
        // Создаем копию для отрисовки
        cv::Mat displayFrame = frame.clone();
        // Получаем текущий ROI (если не выбран - используем весь кадр)
        cv::Rect currentROI = roiSelected ? selectedROI : cv::Rect(0, 0, frame.cols, frame.rows);

        // Рисуем интерфейс в зависимости от режима
        if (roiSelectionMode) {
            // Режим выбора ROI - только показываем видео и рисуем прямоугольник
            if (drawing || (selectedROI.width > 0 && selectedROI.height > 0)) {
                cv::rectangle(displayFrame, selectedROI, cv::Scalar(0, 255, 255), 2);
            }
        } else {
            // Обычный режим - отправляем кадры на распознавание
            auto current_time = std::chrono::steady_clock::now();
            auto time_since_last_ocr = current_time - last_ocr_time;

            // Проверяем, прошла ли 1 секунда с последнего запроса
            if (time_since_last_ocr >= ocr_interval) {
#ifdef DUMP_TO_FILE
                cv::imwrite("frames/frame_" + std::to_string(frameNum) + ".jpg", frame);
#endif

                cv::Mat roiArea = frame(currentROI);
#ifdef DUMP_TO_FILE
                cv::imwrite("frames/frame_" + std::to_string(frameNum) + "_roiArea.jpg", roiArea);
#endif
                roiArea = enlarge_img(roiArea, 150);
                if (!roiArea.empty()) {
#ifdef DUMP_TO_FILE
                    cv::imwrite("frames/frame_" + std::to_string(frameNum)
                                    + "_roiArea_enlarged.jpg",
                                roiArea);
#endif

                    cv::Mat roiPlate = detectPlate(roiArea);
                    if (!roiPlate.empty()) {
                        // Коррекция перекоса
                        auto [angle, alignedPlate] = correct_skew(roiPlate, 1, 15);
                        ocrClient->submitFrameForRecognition(alignedPlate);

                        last_ocr_time = current_time;
                    }
                }
            }
        }

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

void NumberPlateRecognizer::onOCRResultReceived(const QString &plateText, double confidence)
{
    if (!plateText.isEmpty()) {
        std::cout << "=== Detected plate: " << plateText.toStdString()
                  << " (confidence: " << confidence << ") ===" << std::endl;
    }

    // Можно также emit-ить сигнал для MainWindow
    emit plateDetected(plateText, confidence);
}

std::pair<double, cv::Mat> NumberPlateRecognizer::correct_skew(const cv::Mat &image, double delta,
                                                               int limit)
{
    // Функция для вычисления скора
    auto determine_score = [](const cv::Mat &arr, double angle) -> std::pair<cv::Mat, double> {
        cv::Point2f center(arr.cols / 2.0f, arr.rows / 2.0f);
        cv::Mat rotation_matrix = cv::getRotationMatrix2D(center, angle, 1.0);

        cv::Mat data;
        cv::warpAffine(arr, data, rotation_matrix, arr.size(), cv::INTER_NEAREST,
                       cv::BORDER_CONSTANT, cv::Scalar(0));

        // Вычисляем гистограмму (сумма по строкам)
        cv::Mat histogram;
        cv::reduce(data, histogram, 1, cv::REDUCE_SUM, CV_64F);

        // Вычисляем score
        double score = 0.0;
        for (int i = 1; i < histogram.rows; i++) {
            double diff = histogram.at<double>(i) - histogram.at<double>(i - 1);
            score += diff * diff;
        }

        return std::make_pair(histogram, score);
    };

    // Конвертируем в grayscale и бинаризуем
#if 0
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::Mat thresh;
    cv::threshold(gray, thresh, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
#endif

    // Перебираем углы и вычисляем скоре
    std::vector<double> scores;
    std::vector<double> angles;

    for (double angle = -limit; angle <= limit + delta; angle += delta) {
        auto [histogram, score] = determine_score(image, angle);
        scores.push_back(score);
        angles.push_back(angle);
    }

    // Находим угол с максимальным скором
    auto max_iter = std::max_element(scores.begin(), scores.end());
    int best_index = std::distance(scores.begin(), max_iter);
    double best_angle = angles[best_index];

    // Применяем поворот к оригинальному изображению
    cv::Point2f center(image.cols / 2.0f, image.rows / 2.0f);
    cv::Mat M = cv::getRotationMatrix2D(center, best_angle, 1.0);

    cv::Mat corrected;
    cv::warpAffine(image, corrected, M, image.size(), cv::INTER_CUBIC, cv::BORDER_REPLICATE);

    return std::make_pair(best_angle, corrected);
}

cv::Mat NumberPlateRecognizer::enlarge_img(const cv::Mat &image, int scale_percent)
{
    if (image.empty()) {
        return cv::Mat();
    }

    int width = int(image.cols * scale_percent / 100.0);
    int height = int(image.rows * scale_percent / 100.0);
    cv::Size dim(width, height);

    cv::Mat resized_image;
    cv::resize(image, resized_image, dim, 0, 0, cv::INTER_AREA);

    return resized_image;
}
