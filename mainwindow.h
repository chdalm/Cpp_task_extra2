#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QImage>
#include <QPoint>
#include <QElapsedTimer>
#include <QApplication>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QFile>
#include <opencv2/opencv.hpp>
#include "imageprocessor.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void openImage();
    void processImage();

private:
    void setupUI();
    void displayImages();
    QPixmap getScaledPixmap(const QImage &img, QLabel *label);
    QImage cvMatToQImage(const cv::Mat &mat);
    cv::Mat qImageToCvMat(const QImage &image);

    QWidget *centralWidget;
    QLabel *labelOriginal;
    QLabel *labelResult;
    QPushButton *btnOpen;
    QPushButton *btnProcess;
    QTextEdit *textInfo;
    QLabel* currentDraggingLabel;

    QImage originalImage;
    QImage resultImage;
    cv::Mat currentMat;
    ImageProcessor processor;
    
    double scale;
    QPoint offset;
    bool isDragging;
    QPoint lastDragPos;
};

#endif