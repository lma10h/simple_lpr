#pragma once

#include <opencv2/opencv.hpp>

#include <QThread>

#include <iostream>
#include <regex>
#include <vector>

#include "async_ocr_client.h"

class NumberPlateRecognizer : public QObject
{
    Q_OBJECT

public:
    explicit NumberPlateRecognizer(QObject *parent = nullptr);
    ~NumberPlateRecognizer();

    void startProcessing(const QString &url);
    void stopProcessing();
    void enableROISelection();
    void saveROI();
    void clearROI();

    cv::Mat preprocessImage(const cv::Mat &image);
    cv::Mat detectPlate(const cv::Mat &image);
    void detectText(const cv::Mat &image);
    std::string recognizeText(const cv::Mat &plateImage);

    void processIPCamera(const std::string &url);
    void processRTSP(const std::string &rtspUrl);
    void processHTTP(const std::string &httpUrl);

    void handleMouse(int event, int x, int y, int flags);
    static void onMouse(int event, int x, int y, int flags, void *userdata);

signals:
    void finished();
    void error(const QString &error);
    void roiUpdated(int x, int y, int width, int height);
    void plateDetected(const QString &plate, double confidence);

private:
    cv::Mat upscalePlateSimple(const cv::Mat &plate_image, int scale = 2);
    void onOCRResultReceived(const QString &plateText, double confidence);

    cv::CascadeClassifier plateCascade;

    std::atomic<bool> stopFlag{false};

    // Переменные для рисования прямоугольника
    cv::Rect selectedROI;
    bool roiSelectionMode = false;
    bool roiSelected = false;
    bool drawing = false;
    cv::Point startPoint;
    cv::Point endPoint;

    AsyncOCRClient *ocrClient;
    bool isOCRProcessing = false;
};
