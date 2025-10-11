#include "main_window.h"
#include <QMessageBox>

MainWindow::MainWindow(const QString &url, QWidget *parent)
    : QMainWindow(parent)
    , m_url(url)
{
    // Создаем UI
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    btnStart = new QPushButton("Start Processing", this);
    btnROI = new QPushButton("Select ROI", this);
    btnSaveROI = new QPushButton("Save ROI", this);
    btnClearROI = new QPushButton("Clear ROI", this);
    btnStop = new QPushButton("Stop", this);
    statusLabel = new QLabel("Status: Ready", this);

    layout->addWidget(btnStart);
    layout->addWidget(btnROI);
    layout->addWidget(btnSaveROI);
    layout->addWidget(btnClearROI);
    layout->addWidget(btnStop);
    layout->addWidget(statusLabel);

    setCentralWidget(centralWidget);

    // Создаем распознаватель
    recognizer = new NumberPlateRecognizer();
    connect(recognizer, &NumberPlateRecognizer::finished, this, &MainWindow::onRecognizerFinished);
    connect(recognizer, &NumberPlateRecognizer::error, this, &MainWindow::onRecognizerError);
    connect(recognizer, &NumberPlateRecognizer::roiUpdated, this, &MainWindow::onROIUpdated);

    recognizer->moveToThread(&workerThread);
    workerThread.start();

    // Подключаем сигналы и слоты
    connect(btnStart, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(btnROI, &QPushButton::clicked, this, &MainWindow::onROIClicked);
    connect(btnSaveROI, &QPushButton::clicked, this, &MainWindow::onSaveROIClicked);
    connect(btnClearROI, &QPushButton::clicked, this, &MainWindow::onClearROIClicked);
    connect(btnStop, &QPushButton::clicked, this, &MainWindow::onStopClicked);
}

MainWindow::~MainWindow()
{
    workerThread.quit();
    workerThread.wait();
    recognizer->deleteLater();
}

void MainWindow::onStartClicked()
{
    recognizer->startProcessing(m_url);
    statusLabel->setText("Status: Processing...");
    btnStart->setEnabled(false);
    btnStop->setEnabled(true);
}

void MainWindow::onROIClicked()
{
    recognizer->enableROISelection();
    statusLabel->setText("Status: ROI Selection Mode - draw rectangle on video");
}

void MainWindow::onSaveROIClicked()
{
    recognizer->saveROI();
    statusLabel->setText("Status: ROI Saved");
}

void MainWindow::onClearROIClicked()
{
    recognizer->clearROI();
    statusLabel->setText("Status: ROI Cleared");
}

void MainWindow::onStopClicked()
{
    recognizer->stopProcessing();
    statusLabel->setText("Status: Stopped");
    btnStart->setEnabled(true);
    btnStop->setEnabled(false);
}

void MainWindow::onRecognizerFinished()
{
    statusLabel->setText("Status: Finished");
    btnStart->setEnabled(true);
    btnStop->setEnabled(false);
}

void MainWindow::onRecognizerError(const QString &error)
{
    QMessageBox::critical(this, "Error", error);
    statusLabel->setText("Status: Error");
    btnStart->setEnabled(true);
    btnStop->setEnabled(false);
}

void MainWindow::onROIUpdated(int x, int y, int width, int height)
{
    statusLabel->setText(QString("ROI: %1x%2 at (%3,%4)").arg(width).arg(height).arg(x).arg(y));
}
