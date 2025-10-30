#include "async_ocr_client.h"
#include <QBuffer>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

AsyncOCRClient::AsyncOCRClient(QObject *parent)
    : QObject(parent)
{
    manager = new QNetworkAccessManager();
    connect(manager, &QNetworkAccessManager::finished, this, &AsyncOCRClient::onReplyFinished);
}

void AsyncOCRClient::submitFrameForRecognition(const cv::Mat &frame)
{
    if (frame.empty()) {
        emit plateRecognized("", 0.0);
        return;
    }

    try {
        // Конвертируем cv::Mat в JPEG
        std::vector<uchar> buffer;
        cv::imencode(".jpg", frame, buffer, {cv::IMWRITE_JPEG_QUALITY, 70});

        QByteArray imageData(reinterpret_cast<const char *>(buffer.data()), buffer.size());

        QNetworkRequest request(serviceUrl);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
        manager->post(request, imageData);
    } catch (const cv::Exception &e) {
        qDebug() << "OpenCV exception:" << e.what();
        emit plateRecognized("", 0.0);
    } catch (const std::exception &e) {
        qDebug() << "std exception:" << e.what();
        emit plateRecognized("", 0.0);
    }
}

void AsyncOCRClient::onReplyFinished(QNetworkReply *reply)
{
    QString plateText = "";
    double confidence = 0.0;

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
        QJsonArray plates = response.object()["plates"].toArray();

        if (!plates.isEmpty()) {
            QJsonObject plateObj = plates[0].toObject();
            plateText = plateObj["text"].toString();
            confidence = plateObj["confidence"].toDouble();
        }
    } else {
        qDebug() << "OCR request failed:" << reply->errorString();
    }

    emit plateRecognized(plateText, confidence);
    reply->deleteLater();
}
