#pragma once

#include "NumberPlateRecognizer.h"
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(const QString &url, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartClicked();
    void onROIClicked();
    void onSaveROIClicked();
    void onClearROIClicked();
    void onStopClicked();
    void onRecognizerFinished();
    void onRecognizerError(const QString &error);

private:
    void onROIUpdated(int x, int y, int width, int height);

    QThread workerThread;
    NumberPlateRecognizer *recognizer;

    QPushButton *btnStart;
    QPushButton *btnROI;
    QPushButton *btnSaveROI;
    QPushButton *btnClearROI;
    QPushButton *btnStop;
    QLabel *statusLabel;

    QString m_url;
};
