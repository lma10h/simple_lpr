#pragma once

#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <opencv2/opencv.hpp>

class AsyncOCRClient : public QObject
{
    Q_OBJECT
public:
    explicit AsyncOCRClient(QObject *parent = nullptr);
    void submitFrameForRecognition(const cv::Mat &frame);

signals:
    void plateRecognized(const QString &plate, double confidence);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *manager;
    QString serviceUrl = "http://127.0.0.1:5000/recognize";
};
