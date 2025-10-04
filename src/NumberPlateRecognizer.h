#ifndef NUMBERPLATERECOGNIZER_H
#define NUMBERPLATERECOGNIZER_H

#include <iostream>
#include <leptonica/allheaders.h>
#include <opencv2/opencv.hpp>
#include <regex>
#include <tesseract/baseapi.h>
#include <vector>

class NumberPlateRecognizer
{
private:
    tesseract::TessBaseAPI tess;
    cv::CascadeClassifier plateCascade;

public:
    NumberPlateRecognizer();
    ~NumberPlateRecognizer();

    cv::Mat preprocessImage(const cv::Mat &image);
    std::vector<cv::Rect> detectPlates(const cv::Mat &image);
    std::vector<cv::Rect> findPlatesByContours(const cv::Mat &image);
    std::string recognizeText(const cv::Mat &plateImage);
    std::string cleanNumberPlateText(const std::string &text);
    void processIPCamera(const std::string &url);
    void processRTSP(const std::string &rtspUrl);
    void processHTTP(const std::string &httpUrl);
};

#endif
