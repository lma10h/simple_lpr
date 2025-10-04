#ifndef NUMBERPLATERECOGNIZER_H
#define NUMBERPLATERECOGNIZER_H

#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <iostream>
#include <vector>
#include <regex>

class NumberPlateRecognizer {
private:
    tesseract::TessBaseAPI tess;
    cv::CascadeClassifier plateCascade;

public:
    NumberPlateRecognizer();
    ~NumberPlateRecognizer();
    
    cv::Mat preprocessImage(const cv::Mat& image);
    std::vector<cv::Rect> detectPlates(const cv::Mat& image);
    std::vector<cv::Rect> findPlatesByContours(const cv::Mat& image);
    std::string recognizeText(const cv::Mat& plateImage);
    std::string cleanNumberPlateText(const std::string& text);
    void processCamera(int cameraIndex = 0);
};

#endif
