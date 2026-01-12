#include "mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QFileInfo>
#include <QPixmap>
#include <QPainter>
#include <QFile>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , centralWidget(nullptr)
    , labelOriginal(nullptr)
    , labelResult(nullptr)
    , btnOpen(nullptr)
    , btnProcess(nullptr)
    , textInfo(nullptr)
    , scale(1.0)
    , isDragging(false)
{
    setMinimumSize(800, 600);
    
    setupUI();
}

MainWindow::~MainWindow(){}

void MainWindow::setupUI()
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    QWidget *imageContainer = new QWidget(this);
    QHBoxLayout *imageLayout = new QHBoxLayout(imageContainer);

    QGroupBox *originalGroup = new QGroupBox(this);
    QVBoxLayout *originalLayout = new QVBoxLayout(originalGroup);
    
    labelOriginal = new QLabel(this);
    labelOriginal->setAlignment(Qt::AlignCenter);
    labelOriginal->setMinimumSize(400, 300);
    
    originalLayout->addWidget(labelOriginal);
    
    QGroupBox *resultGroup = new QGroupBox(this);
    QVBoxLayout *resultLayout = new QVBoxLayout(resultGroup);
    
    labelResult = new QLabel(this);
    labelResult->setAlignment(Qt::AlignCenter);
    labelResult->setMinimumSize(400, 300);
    
    resultLayout->addWidget(labelResult);
    
    imageLayout->addWidget(originalGroup, 1);
    imageLayout->addWidget(resultGroup, 1);

    QWidget *buttonContainer = new QWidget(this);
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonContainer);
    
    btnOpen = new QPushButton("Выбрать изображение", this);
    connect(btnOpen, &QPushButton::clicked, this, &MainWindow::openImage);
    
    btnProcess = new QPushButton("Запустить", this);
    connect(btnProcess, &QPushButton::clicked, this, &MainWindow::processImage);
    btnProcess->setEnabled(false);
    
    
    buttonLayout->addWidget(btnOpen);
    buttonLayout->addWidget(btnProcess);
    buttonLayout->addStretch();
    
    
    textInfo = new QTextEdit(this);
    textInfo->setMaximumHeight(200);
    
    mainLayout->addWidget(imageContainer, 4);
    mainLayout->addWidget(buttonContainer);
    mainLayout->addWidget(textInfo, 1) ;
    
}

void MainWindow::openImage()
{
    QStringList filters;
    filters << "Image files (*.png *.jpg *.jpeg *.bmp *.tiff *.tif)"
            << "PNG files (*.png)"
            << "JPEG files (*.jpg *.jpeg)"
            << "All files (*.*)";
    
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open Image",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        filters.join(";;")
    );
    
    if (filePath.isEmpty()) return;
    
    QImage loadedImage;
    if (!loadedImage.load(filePath)) {
        QMessageBox::critical(this, "Error", "Cannot load image: " + filePath);
        return;
    }
    
    originalImage = loadedImage.convertToFormat(QImage::Format_RGB888);
    currentMat = qImageToCvMat(originalImage);
    
    resultImage = QImage();
    btnProcess->setEnabled(true);
    
    displayImages();
    
}

void MainWindow::processImage()
{
    if (currentMat.empty()) {
        QMessageBox::critical(this, "Error", "No image loaded!");
        return;
    }
    
    QApplication::processEvents();
    
    QElapsedTimer timer;
    timer.start();
    
    try {
        cv::Mat gray;
        if (currentMat.channels() > 1) {
            cv::cvtColor(currentMat, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = currentMat.clone();
        }
        
        cv::Mat bin;
        cv::threshold(gray, bin, 127, 255, cv::THRESH_BINARY);
        
        processor.process(bin);

        
        
        qint64 ms = timer.elapsed();
        
        cv::Mat out;
        cv::cvtColor(bin, out, cv::COLOR_GRAY2BGR);
        
        const auto& contours = processor.getContours();
        
        for (size_t i = 0; i < contours.size(); ++i) {
            if (contours[i].empty()) continue;
            
            double area = cv::contourArea(contours[i], true);
            
            if (area > 0) {
                cv::drawContours(out, contours, i, cv::Scalar(0, 255, 0), 2);
            } else {

                cv::drawContours(out, contours, i, cv::Scalar(0, 0, 255), 2);
            }
        }
        const auto& cuts = processor.getCuts();
        for (const auto& cut : cuts) {
            cv::line(out, 
                    cv::Point((int)cut.p_out.x, (int)cut.p_out.y),
                    cv::Point((int)cut.p_in.x, (int)cut.p_in.y),
                    cv::Scalar(255, 0, 0), 2); 
            
            cv::circle(out, cut.p_out, 4, cv::Scalar(255, 255, 0), -1);
            
            cv::circle(out, cut.p_in, 4, cv::Scalar(0, 255, 255), -1);
        }
        
        resultImage = cvMatToQImage(out);
        
        
        displayImages();
        
        QString infoStr = QString::fromStdString(processor.getInfoString()) +
                         "Время выполнения: " + QString::number(ms) + " мс";
        textInfo->setText(infoStr);

        
        
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("Processing error: %1").arg(e.what()));
    } catch (...) {
        QMessageBox::critical(this, "Error", "Unknown processing error");
    }
    
}


void MainWindow::displayImages()
{
    if (!originalImage.isNull()) {
        QPixmap pix = getScaledPixmap(originalImage, labelOriginal);
        labelOriginal->setPixmap(pix);
    }
    
    if (!resultImage.isNull()) {
        QPixmap pix = getScaledPixmap(resultImage, labelResult);
        labelResult->setPixmap(pix);
    }
}

QPixmap MainWindow::getScaledPixmap(const QImage &img, QLabel *label)
{
    if (img.isNull() || !label) return QPixmap();

    int scaledWidth = static_cast<int>(img.width() * scale);
    int scaledHeight = static_cast<int>(img.height() * scale);

    QImage scaledImg = img.scaled(scaledWidth, scaledHeight, 
                                  Qt::KeepAspectRatio, Qt::SmoothTransformation);
    
    QPixmap pixmap(label->size());
    pixmap.fill(Qt::gray);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    int x = offset.x();
    int y = offset.y();
    
    QRect labelRect = label->rect();
    if (scaledWidth < labelRect.width()) {
        x = (labelRect.width() - scaledWidth) / 2;
    } else {
        x = qMax(labelRect.width() - scaledWidth, qMin(x, 0));
    }
    
    if (scaledHeight < labelRect.height()) {
        y = (labelRect.height() - scaledHeight) / 2;
    } else {
        y = qMax(labelRect.height() - scaledHeight, qMin(y, 0));
    }
    
    painter.drawImage(x, y, scaledImg);
    
    return pixmap;
}
QImage MainWindow::cvMatToQImage(const cv::Mat &mat)
{
    if (mat.type() == CV_8UC3) {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        return QImage((const unsigned char*)rgb.data, 
                     rgb.cols, rgb.rows, 
                     static_cast<int>(rgb.step), 
                     QImage::Format_RGB888).copy();
    } else if (mat.type() == CV_8UC1) {
        return QImage((const unsigned char*)mat.data, 
                     mat.cols, mat.rows, 
                     static_cast<int>(mat.step), 
                     QImage::Format_Grayscale8).copy();
    }
    return QImage();
}

cv::Mat MainWindow::qImageToCvMat(const QImage &image)
{
    QImage converted = image.convertToFormat(QImage::Format_RGB888);
    
    cv::Mat mat(converted.height(), converted.width(), CV_8UC3,
                const_cast<uchar*>(converted.bits()),
                static_cast<size_t>(converted.bytesPerLine()));
    
    return mat.clone();
}


void MainWindow::wheelEvent(QWheelEvent *event)
{
    if (originalImage.isNull()) return;
    
    QPoint numDegrees = event->angleDelta() / 8;
    
    if (!numDegrees.isNull()) {
        double zoomFactor = 1.1;
        
        QPoint mousePos;
        if (labelOriginal->underMouse()) {
            mousePos = labelOriginal->mapFromParent(event->position().toPoint());
        } else if (labelResult->underMouse()) {
            mousePos = labelResult->mapFromParent(event->position().toPoint());
        } else {
            return;
        }
        
        QPointF imagePosBefore((mousePos.x() - offset.x()) / scale,
                               (mousePos.y() - offset.y()) / scale);
        
        if (numDegrees.y() > 0) {
            scale *= zoomFactor;
        } else {
            scale /= zoomFactor;
        }
        
        scale = qMax(0.1, qMin(scale, 10.0));
        
        offset.setX(mousePos.x() - imagePosBefore.x() * scale);
        offset.setY(mousePos.y() - imagePosBefore.y() * scale);
        
        displayImages();
    }
    
    event->accept();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if ((labelOriginal->underMouse() || labelResult->underMouse()) &&
            !originalImage.isNull()) {
            isDragging = true;
            lastDragPos = mapFromGlobal(event->globalPosition().toPoint());
            labelOriginal->setCursor(Qt::ClosedHandCursor);
            labelResult->setCursor(Qt::ClosedHandCursor);
        }
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (isDragging && !originalImage.isNull()) {
        QPoint currentPos = mapFromGlobal(event->globalPosition().toPoint());
        QPoint delta = currentPos - lastDragPos;
        offset += delta;
        lastDragPos = currentPos;
        displayImages();
    }
    QMainWindow::mouseMoveEvent(event);
}
void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isDragging) {
        isDragging = false;
        labelOriginal->setCursor(Qt::OpenHandCursor);
        labelResult->setCursor(Qt::OpenHandCursor);
    }
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    displayImages();
}
