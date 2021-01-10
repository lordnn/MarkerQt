/**
 * The MIT License (MIT)
 * Copyright (c) 2017-2018 Kirill Lebedev
**/

#ifndef RENDERAREA_H
#define RENDERAREA_H

#include "base.h"
#include "renderthread.h"
#include <QLoggingCategory>
#include <QImage>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QTabWidget>
#include <array>
#include <memory>
#include <vector>

//#include <QAbstractVideoSurface>
//#include <QMediaPlayer>
//#include <QVideoSurfaceFormat>

// forward declarations
QT_BEGIN_NAMESPACE
class QGestureEvent;
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
class QMenu;
class QPinchGesture;
QT_END_NAMESPACE
class RenderArea;
class CornerGrabber;

Q_DECLARE_LOGGING_CATEGORY(lcExample)

class PointItem : public QGraphicsItem
{
public:
    enum { Type = UserType + 4 };

    explicit PointItem(int n) : num_(n)
    { }
    int getNum() const {
        return num_;
    }

    QRectF boundingRect() const Q_DECL_OVERRIDE;
    void paint(QPainter *paint, const QStyleOptionGraphicsItem *option, QWidget *widget) Q_DECL_OVERRIDE;
    int type() const Q_DECL_OVERRIDE {
        return Type;
    }

private:
    int num_;
    qreal _m11 = 1.;
};

class RectItem : public QGraphicsItem
{
public:
    enum { Type = UserType + 5 };

    explicit RectItem(const QRect &r);
    QRect getRect() const;

    QRectF boundingRect() const Q_DECL_OVERRIDE;
    void paint(QPainter *paint, const QStyleOptionGraphicsItem *option, QWidget *widget) Q_DECL_OVERRIDE;
    int type() const Q_DECL_OVERRIDE {
        return Type;
    }

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) Q_DECL_OVERRIDE;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) Q_DECL_OVERRIDE;

    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) Q_DECL_OVERRIDE;

private:
    void setCornerPositions();
    void adjustSize(qreal x, qreal y);

    QColor _outterborderColor;
    QPen _outterborderPen;
    QBrush _outterborderBrush;
    QPointF _location, _dragStart;
    qreal _width = 400, _height = 400;
    constexpr static int _XcornerGrabBuffer = 8, _YcornerGrabBuffer = 8;
    qreal _drawingWidth = _width - _XcornerGrabBuffer, _drawingHeight = _height - _YcornerGrabBuffer;
    constexpr static qreal _drawingOrigenX = _XcornerGrabBuffer, _drawingOrigenY = _YcornerGrabBuffer;

    std::array<std::unique_ptr<CornerGrabber>, 4> _corners;
};

class RenderArea : public QGraphicsView
{
    Q_OBJECT
public:
    RenderArea(QGraphicsScene *scene);
protected:
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;
#endif
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    int getNextNum() const;
private:
    qreal h11 = 1., h12 = 0., h21 = 0., h22 = 1.;
    const qreal _scaleStep = 1.41421356;
    int _scaleFactor = 0;
    bool _bPan = false;
    QPoint _panStart;
};

/*
class MyVideoSurface : public QAbstractVideoSurface
{
    Q_OBJECT
public:
    MyVideoSurface(RenderArea *renderArea) : renderArea_(renderArea), bFrameProcessed_(true)
    { }

    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const Q_DECL_OVERRIDE
    {
        Q_UNUSED(handleType);

        return QList<QVideoFrame::PixelFormat>()
                << QVideoFrame::Format_RGB32
                << QVideoFrame::Format_ARGB32
                << QVideoFrame::Format_RGB565
                << QVideoFrame::Format_RGB555;
    }

    bool start(const QVideoSurfaceFormat &format) Q_DECL_OVERRIDE
    {
        bool bRes{false};
        if (isFormatSupported(format)) {
            imageFormat_ = QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat());
            imageSize_ = format.frameSize();
            QAbstractVideoSurface::start(format);
            bRes = true;
        }
        return bRes;
    }

    bool present(const QVideoFrame &frame) Q_DECL_OVERRIDE;

public slots:
    void sltFaceDetector()
    {
        bFrameProcessed_ = true;
    }

private:
    QImage::Format imageFormat_;
    QSize imageSize_;
    RenderArea *renderArea_;
    bool bFrameProcessed_;
};

class RenderArea : public QGraphicsView
{
    Q_OBJECT
public:
    RenderArea(QGraphicsScene *scene);
    ~RenderArea() Q_DECL_OVERRIDE;

    void openFile(const QString &filename);
    void assignImage(const QImage &image);

signals:
    void positionChanged(QString str);
    void pointsChanged(QString str);
    void painted();

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;
#endif
#ifndef ANDROID
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
#endif
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    bool event(QEvent *) Q_DECL_OVERRIDE;

private slots:
    void updatePixmap(const QImage &image, QPointF center, double scaleFactor);
    void sltNoMemory();
    void sltDrawRect(bool bChecked);
    void sltFaceDetector(bool bChecked);
    void sltFaceDetector(CRectArray &arr);
    void sltLBFRDetector(bool bChecked);
    void sltLBFRDetector(CPointFArray &arr);

public:
	void zoom(QPoint pos, double zoomFactor);
	void scroll(int deltaX, int deltaY);
private:
	bool gestureEvent(QGestureEvent *);
	void pinchTriggered(QPinchGesture *);

    void fFaceDetector();
    void fLBFRDetector();

    RenderThread _thread;
	QPixmap pixmap;
    QPoint lastDragPos, rectDragPos;
	QPointF pixmapCenter, screenCenter;
	double pixmapScale, screenScale;
    QImage image_;
    std::vector<QRect> frects;
    QRectF frect_;
    std::vector<QPointF> pts;
    bool _bFaceChecked = false, _bLBFRChecked = false, _bDrawRect = false, _bStartRect = false;
    //QMediaPlayer mediaPlayer;
    //MyVideoSurface *videoItem;
};
*/
#endif // RENDERAREA_H
