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
        cv::imencode(".jpg", frame, buffer, {cv::IMWRITE_JPEG_QUALITY, 80});

        QByteArray byteArray(reinterpret_cast<const char *>(buffer.data()), buffer.size());
        QByteArray base64Data = byteArray.toBase64();

        // Дальше без изменений
        QJsonObject requestData;
        requestData["image"] = QString(base64Data);

        QNetworkRequest request(serviceUrl);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonDocument doc(requestData);
        manager->post(request, doc.toJson());
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
            qDebug() << "OCR result:" << plateText << confidence;
        }
    } else {
        qDebug() << "OCR request failed:" << reply->errorString();
    }

    emit plateRecognized(plateText, confidence);
    reply->deleteLater();
}
