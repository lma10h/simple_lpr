#pragma once

#include <iostream>
#include <leptonica/allheaders.h>
#include <opencv2/opencv.hpp>
#include <regex>
#include <tesseract/baseapi.h>
#include <vector>

#include <QThread>

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
    std::vector<cv::Rect> detectPlates(const cv::Mat &image);
    void detectText(const cv::Mat &image, const std::vector<cv::Rect> &plates);
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

private:
    tesseract::TessBaseAPI tess;
    cv::CascadeClassifier plateCascade;

    std::atomic<bool> stopFlag{false};

    // Переменные для рисования прямоугольника
    cv::Rect selectedROI;
    bool roiSelectionMode = false;
    bool roiSelected = false;
    bool drawing = false;
    cv::Point startPoint;
    cv::Point endPoint;
};
