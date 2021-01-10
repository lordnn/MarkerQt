/**
 * The MIT License (MIT)
 * Copyright (c) 2017-2018 Kirill Lebedev
**/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "base.h"

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTabWidget>

#include <memory>
#include <vector>


// forward declarations
QT_BEGIN_NAMESPACE
class QLabel;
class QSlider;
class QGraphicsRectItem;
class QGraphicsEllipseItem;
QT_END_NAMESPACE
class RenderArea;
class RectItem;


class TabWidget : public QTabWidget
{
    Q_OBJECT
public:
    TabWidget(QWidget *parent = nullptr);
private:
    friend class MainWindow;
    RenderArea *renderArea;
};

class VideoStream;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow() Q_DECL_OVERRIDE;

    void loadFile(const QString &);
protected:
    //void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;

private slots:
    void fFaceDetector();
    void fLBFRDetector();

    void sltNoMemory();
    void opnFile();
    void impFile();
    void expFile();
    void expRect();
    void updateStatusBar(const QString &str);
    void sltFaceDetector(bool bChecked);
    void sltFaceDetector(CRectArray &arr);
    void sltLBFRDetector(bool bChecked);
    void sltLBFRDetector(CPointFArray &arr);
    void nextFrame();
    void prevFrame();
    void sltRotation0();
    void sltRotation90();
    void sltRotation270();
    void sltAddRect();
    void sltAbout();

private:
    enum class Rotation { Rot0, Rot90, Rot180, Rot270 };
    void AddPoint(const QPointF &p);
    void AddRect(const QRect &r = QRect(0, 0, 60, 60));
    void Rotate();

    int ptNum = 0;
    QGraphicsScene _scene;
    QGraphicsView *_gview = nullptr;
    QImage _image0, _image;
    //RenderArea *renderArea = nullptr;
    QSlider *slider;
    QLabel *posLabel;
    std::vector<RectItem*> _rects;
    std::vector<QGraphicsEllipseItem*> _points;
    std::unique_ptr<VideoStream> _safeStream;
    Rotation rotation_{Rotation::Rot0};
};

#endif // MAINWINDOW_H
