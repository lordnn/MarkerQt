/**
 * The MIT License (MIT)
 * Copyright (c) 2017-2018 Kirill Lebedev
**/

#include "renderarea.h"
#include "cornergrabber.h"
#include "mainwindow.h"
#include "worker.h"

#include <random>
#include <time.h>
#include <QDir>
#include <QFile>
#include <QGestureEvent>
#include <QGraphicsSceneEvent>
#include <QGuiApplication>
#include <QImageReader>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QShortcut>
#include <QTabBar>

constexpr double DefaultScale{1.0};

extern const double ZoomInFactor{0.8};
extern const double ZoomOutFactor{1 / ZoomInFactor};
extern const int ScrollStep{20};

Q_LOGGING_CATEGORY(lcExample, "QtMarker")

QRectF PointItem::boundingRect() const {
    qreal dx(5);
    if (_m11 < 1.) {
        dx /= _m11;
    }
    return QRectF(-dx, -dx, 2 * dx, 2 * dx);
}

void PointItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    QTransform t = painter->transform();
    _m11 = t.m11();
    qreal dx(3);
    if (_m11 < 1.) {
        dx /= _m11;
    }
    if (this->isSelected()) {
        qreal width(2.5);
        if (_m11 < 1.) {
            width /= _m11;
        }
        painter->setPen(QPen(Qt::black, width));
        painter->drawLine(QPointF(-dx, -dx), QPointF(dx, dx));
        painter->drawLine(QPointF(-dx, dx), QPointF(dx, -dx));
    }
    qreal width(2);
    if (_m11 < 1.) {
        width /= _m11;
    }
    painter->setPen(QPen(Qt::red, width));
    painter->drawLine(QPointF(-dx, -dx), QPointF(dx, dx));
    painter->drawLine(QPointF(-dx, dx), QPointF(dx, -dx));
    if (_m11 > 4.) {
        painter->save();
        painter->setTransform(QTransform(1, t.m12(), t.m12(), t.m21(), 1, t.m23(), t.m31(), t.m32(), t.m33()));
        //auto font = painter->font();
        //font.setPixelSize();
        QPainterPath path;
        path.addText(0, 0, QFont("Tahoma", 10, QFont::Bold), QString("%1").arg(num_));
        //painter->setFont(QFont("Tahoma", 10, QFont::Bold));
        painter->setPen(QPen(Qt::black, 4));
        //painter->drawText(0, 0, QString("%1").arg(num_));
        painter->drawPath(path);
        painter->fillPath(path, QBrush(Qt::white));
        //painter->drawText(QRectF(0., 0., 2. * m11, 1. * m11), QString("%1").arg(num_));
        painter->restore();
    }
}

RectItem::RectItem(const QRect &r) : _outterborderColor(Qt::red), _outterborderPen(_outterborderColor, 2), _outterborderBrush(QColor(150, 150, 150, 125), Qt::NoBrush), _width(r.width()), _height(r.height()) {
    this->setPos(r.topLeft());
    this->setAcceptHoverEvents(true);
}

QRect RectItem::getRect() const {
    return QRect(QPoint(std::round(this->pos().x()), std::round(this->pos().y())), QSize(_width, _height));
}

QRectF RectItem::boundingRect() const {
    return QRectF(-_XcornerGrabBuffer, -_YcornerGrabBuffer, _width + 2 * _XcornerGrabBuffer, _height + 2 * _YcornerGrabBuffer);
}

bool RectItem::sceneEventFilter(QGraphicsItem *watched, QEvent *event) {
    qDebug() << " QEvent == " + QString::number(event->type());

    CornerGrabber *corner = dynamic_cast<CornerGrabber *>(watched);
    if (nullptr == corner) {
        return false;
    }

    QGraphicsSceneMouseEvent *mevent = dynamic_cast<QGraphicsSceneMouseEvent *>(event);
    if (nullptr == mevent) {
        return false;
    }

    switch (event->type())
    {
    case QEvent::GraphicsSceneMousePress:
        corner->setMouseState(CornerGrabber::KMouse::kMouseDown);
        corner->mouseDownX = mevent->pos().x();
        corner->mouseDownY = mevent->pos().y();
        break;
    case QEvent::GraphicsSceneMouseRelease:
        corner->setMouseState(CornerGrabber::KMouse::kMouseReleased);
        break;
    case QEvent::GraphicsSceneMouseMove:
        corner->setMouseState(CornerGrabber::KMouse::kMouseMoving);
        break;
    default:
        return false;
        break;
    }
    if (CornerGrabber::KMouse::kMouseMoving == corner->getMouseState()) {
        qreal x{mevent->pos().x()}, y{mevent->pos().y()};

        int XaxisSign{}, YaxisSign{};
        switch (corner->getCorner())
        {
        case 0:
            XaxisSign = 1;
            YaxisSign = 1;
            break;
        case 1:
            XaxisSign = -1;
            YaxisSign = 1;
            break;
        case 2:
            XaxisSign = -1;
            YaxisSign = -1;
            break;
        case 3:
            XaxisSign = 1;
            YaxisSign = -1;
            break;
        }

        const qreal xMoved{corner->mouseDownX - x};
        const qreal yMoved{corner->mouseDownY - y};

        const qreal newWidth{std::max(_width + (XaxisSign * xMoved), static_cast<qreal>(40))};
        const qreal newHeight{std::max(_height + (YaxisSign * yMoved), static_cast<qreal>(40))};

        qreal deltaWidth{newWidth - _width};
        qreal deltaHeight{newHeight - _height};

        const auto oldRect{this->boundingRect()};

        adjustSize(deltaWidth, deltaHeight);

        deltaWidth *= -1;
        deltaHeight *= -1;

        if (0 == corner->getCorner()) {
            const qreal newXpos = this->pos().x() + deltaWidth;
            const qreal newYpos = this->pos().y() + deltaHeight;
            this->setPos(newXpos, newYpos);
        }
        else if (1 == corner->getCorner()) {
            const qreal newYpos = this->pos().y() + deltaHeight;
            this->setPos(this->pos().x(), newYpos);
        }
        else if (3 == corner->getCorner()) {
            const qreal newXpos = this->pos().x() + deltaWidth;
            this->setPos(newXpos, this->pos().y());
        }

        setCornerPositions();
        scene()->update(this->mapToScene(oldRect.united(this->boundingRect())).boundingRect());
    }
    return true;
}

void RectItem::adjustSize(qreal x, qreal y) {
    _width += x;
    _height += y;

    _drawingWidth = _width - _XcornerGrabBuffer;
    _drawingHeight = _height - _YcornerGrabBuffer;
}

void RectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setPen(_outterborderPen);
    painter->setBrush(_outterborderBrush);
    painter->drawRect(0, 0, _width, _height);
#if 0
    QLinearGradient gradient;
    gradient.setStart(0, 0);
    gradient.setFinalStop(_width, 0);
    QColor grey1(150, 150, 150, 125);
    QColor grey2(225, 225, 225, 125);

    gradient.setColorAt((qreal)0, grey1);
    gradient.setColorAt((qreal)1, grey2);

    QBrush brush(gradient);

    painter->setBrush(brush);

    painter->setPen(_outterborderPen);

    QPointF topLeft(0, 0);
    QPointF bottomRight(_width, _height);

    QRectF rect(topLeft, bottomRight);
    painter->drawRoundRect(rect, 25, 25);
#endif
}

void RectItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
    _outterborderBrush.setStyle(Qt::NoBrush);
    std::for_each(std::begin(_corners), std::end(_corners), [](auto &e){ e->setParentItem(nullptr); e.reset(nullptr); });
    this->update(0, 0, _width, _height);
}

void RectItem::hoverEnterEvent(QGraphicsSceneHoverEvent *) {
    _outterborderBrush.setStyle(Qt::SolidPattern);
    _corners[0].reset(new CornerGrabber(this, 0));
    _corners[1].reset(new CornerGrabber(this, 1));
    _corners[2].reset(new CornerGrabber(this, 2));
    _corners[3].reset(new CornerGrabber(this, 3));

    std::for_each(std::begin(_corners), std::end(_corners), [this](auto &e){ e->installSceneEventFilter(this); });

    setCornerPositions();
    this->update(0, 0, _width, _height);
}

void RectItem::setCornerPositions() {
    _corners[0]->setPos(-_XcornerGrabBuffer, -_YcornerGrabBuffer);
    _corners[1]->setPos(_width + _XcornerGrabBuffer - _corners[1]->boundingRect().width(), -_YcornerGrabBuffer);
    _corners[2]->setPos(_width + _XcornerGrabBuffer - _corners[2]->boundingRect().width(), _height + _YcornerGrabBuffer - _corners[2]->boundingRect().height());
    _corners[3]->setPos(-_drawingOrigenX, _height + _YcornerGrabBuffer - _corners[3]->boundingRect().height());
}

/*bool MyVideoSurface::present(const QVideoFrame &frame)
{
    Q_UNUSED(frame);

    bool bRes{false};
    if (bFrameProcessed_) {
        QVideoFrame currentFrame(frame);
        auto f = currentFrame.pixelFormat();
        if (currentFrame.map(QAbstractVideoBuffer::ReadOnly)) {
            bFrameProcessed_ = false;
            //renderArea_->assignImage(QImage(currentFrame.bits(), imageSize_.width(), imageSize_.height(), imageFormat_));
            currentFrame.unmap();
        }
        bRes = true;
    }

    return bRes;
}*/

RenderArea::RenderArea(QGraphicsScene *scene) : QGraphicsView(scene) {
    this->setDragMode(QGraphicsView::RubberBandDrag);
    this->setRubberBandSelectionMode(Qt::IntersectsItemBoundingRect);
}

/*void RenderArea::wheelEvent(QWheelEvent *e) {
    if ((e->modifiers() & Qt::ControlModifier) == Qt::ControlModifier && e->angleDelta().x() == 0) {
        QPoint pos = e->pos();
        QPointF posf = this->mapToScene(pos);

        double by;
        double angle = e->angleDelta().y();
        if (angle > 0) { by = 1 + (angle / 360 * 0.1); }
        else if (angle < 0) { by = 1 - (-angle / 360 * 0.1); }
        else {
            by = 1;
        }
        scale(by, by);

        double w = viewport()->width();
        double h = viewport()->height();

        double wf = mapToScene(QPoint(w - 1, 0)).x() - mapToScene(QPoint(0,0)).x();
        double hf = mapToScene(QPoint(0, h - 1)).y() - mapToScene(QPoint(0,0)).y();

        double lf = posf.x() - pos.x() * wf / w;
        double tf = posf.y() - pos.y() * hf / h;

        ensureVisible(lf, tf, wf, hf, 0, 0);

        QPointF newPos = mapToScene(pos);

        ensureVisible(QRectF(QPointF(lf, tf) - newPos + posf, QSizeF(wf, hf)), 0, 0);
        e->accept();
    }
    if ((e->modifiers() & Qt::ControlModifier) != Qt::ControlModifier) {
        QGraphicsView::wheelEvent(e);
    }
}*/

void RenderArea::wheelEvent(QWheelEvent *event) {
    const int degrees = event->delta() / 8;
    int steps = degrees / 15;
    constexpr int minScaleFactor = -8;
    constexpr int maxScaleFactor = 10;
    if (steps > 0) {
        _scaleFactor = std::min(maxScaleFactor, _scaleFactor + 1);
    }
    else {
        _scaleFactor = std::max(minScaleFactor, _scaleFactor - 1);
    }
    h11 = h22 = std::pow(_scaleStep, _scaleFactor);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setTransform(QTransform(h11, h12, h21, h22, 0, 0));
}

void RenderArea::mousePressEvent(QMouseEvent *event) {
    if (Qt::RightButton == event->button()) {
        _bPan = true;
        _panStart = event->pos();
        this->setCursor(Qt::ClosedHandCursor);
        event->accept();
    }
    else if (Qt::LeftButton == event->button() && Qt::ControlModifier == event->modifiers()) {
        auto n = this->getNextNum();
        auto *item = new PointItem(n);
        item->setPos(mapToScene(event->pos()));
        item->setFlag(QGraphicsItem::GraphicsItemFlag::ItemIsMovable, true);
        item->setFlag(QGraphicsItem::GraphicsItemFlag::ItemIsSelectable, true);
        this->scene()->addItem(item);
        event->accept();
    }
    else if (Qt::LeftButton == event->button() && Qt::ShiftModifier == event->modifiers()) {
        if (auto *item = dynamic_cast<PointItem*>(this->itemAt(event->pos()))) {
            this->scene()->removeItem(item);
        }
        else if (auto *item = dynamic_cast<QGraphicsRectItem*>(this->itemAt(event->pos()))) {
            this->scene()->removeItem(item);
        }
        event->accept();
    }
    else {
        QGraphicsView::mousePressEvent(event);
    }
}

void RenderArea::mouseReleaseEvent(QMouseEvent *event) {
    if (Qt::RightButton == event->button()) {
        _bPan = false;
        this->setCursor(Qt::ArrowCursor);
        event->accept();
    }
    else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

void RenderArea::mouseMoveEvent(QMouseEvent *event) {
    if (_bPan) {
        this->horizontalScrollBar()->setValue(horizontalScrollBar()->value() - (event->x() - _panStart.x()));
        this->verticalScrollBar()->setValue(verticalScrollBar()->value() - (event->y() - _panStart.y()));
        _panStart = event->pos();
        event->accept();
    }
    else {
        QGraphicsView::mouseMoveEvent(event);
    }
}

int RenderArea::getNextNum() const {
    auto items = this->items();
    std::vector<bool> v(items.size());
    for (auto it = items.cbegin(); it != items.cend(); ++it) {
        if (auto p = dynamic_cast<PointItem*>(*it)) {
            v[p->getNum()] = true;
        }
    }
    int i = 0;
    for (; i < v.size(); ++i) {
        if (false == v[i]) {
            break;
        }
    }
    return i;
}

#if 0
RenderArea::RenderArea(QGraphicsScene *scene) : QGraphicsView(scene), pixmapScale(DefaultScale), screenScale(DefaultScale)
{
    //setTabPosition(QTabWidget::South);
    /*QTabBar *tabBar = this->tabBar();
    tabBar->addTab("Rect");
    tabBar->addTab("LBFR");*/

    qRegisterMetaType<CRectArray>("CRectArray&");
    qRegisterMetaType<std::vector<QPointF>>("CPointFArray&");

    setPalette(QPalette(Qt::white));
    setAutoFillBackground(true);
    connect(&_thread, SIGNAL(renderedImage(const QImage&, QPointF, double)), this, SLOT(updatePixmap(const QImage&, QPointF, double)));
    setMouseTracking(true);
    grabGesture(Qt::PanGesture);
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::TapGesture);


    //setTabBar(m_tabBar);
    /*videoItem = new MyVideoSurface(this);
    mediaPlayer.setVideoOutput(videoItem);
    auto url = QUrl::fromLocalFile("D:\\Projects\\Marker\\20161115_172111.mp4");
    mediaPlayer.setMedia(QUrl::fromLocalFile("D:\\Data\\Video\\641.avi"));
    mediaPlayer.play();

    connect(this, SIGNAL(painted()), videoItem, SLOT(sltFaceDetector()));*/
}

RenderArea::~RenderArea()
{
    /*videoItem->stop();
    videoItem->deleteLater();
    mediaPlayer.stop();*/
}

void RenderArea::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (screenScale == pixmapScale && screenCenter == pixmapCenter && size() == pixmap.size())
        painter.drawPixmap(QPoint(), pixmap);
    else {
        const double scaleFactor = pixmapScale / screenScale;

        double newX = pixmapCenter.x() - pixmap.width() / 2 * pixmapScale;
        double newY = pixmapCenter.y() - pixmap.height() / 2 * pixmapScale;
        newX = size().width() / 2 + (newX - screenCenter.x()) / screenScale;
        newY = size().height() / 2 + (-screenCenter.y() + newY) / screenScale;

        painter.save();
        painter.translate(newX, newY);
        painter.scale(scaleFactor, scaleFactor);
        QRectF exposed = painter.matrix().inverted().mapRect(rect()).adjusted(-1, -1, 1, 1);
        painter.drawPixmap(exposed, pixmap, exposed);
        painter.restore();
    }
    painter.drawRect(frect_);
    //mediaPlayer.play();
    emit painted();
}

void RenderArea::updatePixmap(const QImage &image, QPointF center, double scaleFactor)
{
    if (!lastDragPos.isNull())
        return;

    if (screenScale != scaleFactor || screenCenter != center || size() != image.size())
        return;

    pixmap = QPixmap::fromImage(image);
    pixmapScale = scaleFactor;
    pixmapCenter = center;
    update();
}

void RenderArea::resizeEvent(QResizeEvent * /* event */)
{
    _thread.render(screenCenter, screenScale, this->size(), image_, frects, pts);
}

void RenderArea::showEvent(QShowEvent * /* event */)
{
    screenCenter = QPointF(image_.width() / 2.f, image_.height() / 2.f);
    screenScale = std::max(static_cast<decltype(screenScale)>(image_.width()) / this->width(), static_cast<decltype(screenScale)>(image_.height()) / this->height());
}

#ifndef ANDROID
void RenderArea::mousePressEvent(QMouseEvent *event)
{
    if (!_bStartRect && Qt::RightButton == event->button()) {
        lastDragPos = event->pos();
    }
    else if (_bDrawRect && Qt::LeftButton == event->button()) {
        rectDragPos = event->pos();
        _bStartRect = true;
    }
}
#endif

void RenderArea::openFile(const QString &filename)
{
#if 0
    auto url = QUrl::fromLocalFile("D:\\Projects\\Marker\\20161115_172111.mp4");
    mediaPlayer.setMedia(QUrl::fromLocalFile("D:\\Data\\Video\\641.avi"));
    mediaPlayer.play();
#else
    QImageReader reader(filename);
    reader.setAutoTransform(true);
    const QImage newImage = reader.read();
    if (newImage.isNull()) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(), tr("Cannot load %1: %2").arg(QDir::toNativeSeparators(filename), reader.errorString()));
        return;
    }
    image_ = newImage;
    screenCenter = QPointF(image_.width() / 2.f, image_.height() / 2.f);
    screenScale = std::max(static_cast<decltype(screenScale)>(image_.width()) / this->width(), static_cast<decltype(screenScale)>(image_.height()) / this->height());

    if (_bFaceChecked) {
        fFaceDetector();
    }
    if (_bLBFRChecked) {
        fLBFRDetector();
    }
    _thread.render(screenCenter, screenScale, this->size(), image_, frects, pts);
#endif
}

void RenderArea::assignImage(const QImage &image)
{
    //mediaPlayer.pause();
    image_ = image;
    screenCenter = QPointF(image_.width() / 2.f, image_.height() / 2.f);
    screenScale = std::max(static_cast<decltype(screenScale)>(image_.width()) / this->width(), static_cast<decltype(screenScale)>(image_.height()) / this->height());

    if (_bFaceChecked) {
        fFaceDetector();
    } else {
        _thread.render(screenCenter, screenScale, this->size(), image_, frects, pts);
    }
    /*if (_bLBFRChecked) {
        fLBFRDetector();
    }*/
    //_thread.render(screenCenter, screenScale, this->size(), image_, frects, pts);
}

void RenderArea::mouseMoveEvent(QMouseEvent *event)
{
    if (!_bStartRect && event->buttons() & Qt::RightButton) {
        screenCenter -= QPointF((event->pos().x() - lastDragPos.x()) * screenScale, (event->pos().y() - lastDragPos.y()) * screenScale);
        lastDragPos = event->pos();
        update();
    }
    else if (_bStartRect && event->buttons() & Qt::LeftButton) {
        frect_ = QRectF(rectDragPos, event->pos());
        update();
    }
}

void RenderArea::mouseReleaseEvent(QMouseEvent *event)
{
    if (!_bStartRect && event->button() == Qt::RightButton) {
        int deltaX = event->pos().x() - lastDragPos.x();
        int deltaY = event->pos().y() - lastDragPos.y();
        lastDragPos = QPoint();
        scroll(deltaX, deltaY);
    }
    else if (_bStartRect && event->button() == Qt::LeftButton) {
        frect_ = QRectF(rectDragPos, event->pos());
        _bStartRect = false;
    }
}

#ifndef QT_NO_WHEELEVENT
void RenderArea::wheelEvent(QWheelEvent *event)
{
    if (!_bStartRect) {
        int numDegrees = event->delta() / 8;
        double numSteps = numDegrees / 15.0f;
        zoom(event->pos(), pow(ZoomInFactor, numSteps));
    }
}
#endif

void RenderArea::zoom(QPoint pos, double zoomFactor)
{
    QPointF pt((pos.x() - width() / 2) * screenScale, (pos.y() - height() / 2) * screenScale);
    screenScale *= zoomFactor;
    screenCenter += QPointF(pt.x() * (1 - zoomFactor), pt.y() * (1 - zoomFactor));
    update();
    _thread.render(screenCenter, screenScale, size(), image_, frects, pts);
}

void RenderArea::scroll(int deltaX, int deltaY)
{
    screenCenter -= QPointF(deltaX * screenScale, deltaY * screenScale);
    //screenCenter.ry() = screenCenter.y() - image_.height();
    _thread.render(screenCenter, screenScale, this->size(), image_, frects, pts);
}

void RenderArea::sltNoMemory()
{
    QMessageBox::warning(this, "Warning", "No enough memory");
}


void RenderArea::sltDrawRect(bool bChecked)
{
    _bDrawRect = bChecked;
}

void RenderArea::sltFaceDetector(bool bChecked)
{
    Q_UNUSED(bChecked);
    fFaceDetector();
    /*if (_bFaceChecked = bChecked) {
        fFaceDetector();
    }
    else {
        frects.clear();
        _thread.render(screenCenter, screenScale, this->size(), image_, frects, pts);
    }*/
}

void RenderArea::sltFaceDetector(CRectArray &arr)
{
    frects.assign(std::cbegin(arr), std::cend(arr));
    _thread.render(screenCenter, screenScale, this->size(), image_, frects, pts);
    //std::copy(std::begin(arr), std::end(arr), std::back_inserter(frects));
    /*if (_bFaceChecked) {
        frects = std::move(arr);
        _thread.render(screenCenter, screenScale, this->size(), image_, frects, pts);
    }*/
}

void RenderArea::fFaceDetector()
{
    frects.clear();
    auto safeWorker = std::make_unique<TWorker>(TWorker::workerType::wtFaceDetector);
    QThread *thread = new QThread();
    //TWorker *worker = new TWorker(TWorker::workerType::wtFaceDetector);
    TWorker *worker = safeWorker.release();
    worker->setData(image_);
    worker->moveToThread(thread);

    connect(thread, SIGNAL(started()), worker, SLOT(process()));
    connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
    connect(worker ,SIGNAL(completeFaceDetector(CRectArray&)), this, SLOT(sltFaceDetector(CRectArray&)));
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(worker, SIGNAL(noMemory()), this, SLOT(sltNoMemory()));
    //connect(worker, SIGNAL(finished()), videoItem, SLOT(sltFaceDetector()));

    thread->start();
}

void RenderArea::sltLBFRDetector(bool bChecked)
{
    if (_bLBFRChecked = bChecked) {
        fLBFRDetector();
    }
    else {
        pts.clear();
        _thread.render(screenCenter, screenScale, this->size(), image_, frects, pts);
    }
}

void RenderArea::sltLBFRDetector(CPointFArray &arr)
{
    if (_bLBFRChecked) {
        pts = std::move(arr);
        _thread.render(screenCenter, screenScale, this->size(), image_, frects, pts);
    }
}

void RenderArea::fLBFRDetector()
{
    pts.clear();
    auto safeWorker = std::make_unique<TWorker>(TWorker::workerType::wtLBFRDetector);
    QThread *thread = new QThread();
    //TWorker *worker = new TWorker(TWorker::workerType::wtLBFRDetector);
    TWorker *worker = safeWorker.release();
    worker->setData(image_);
    worker->moveToThread(thread);

    connect(thread, SIGNAL(started()), worker, SLOT(process()));
    connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
    connect(worker ,&TWorker::completeLBFRDetector, this, static_cast<void (RenderArea::*)(CPointFArray&)>(&RenderArea::sltLBFRDetector));
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(worker, SIGNAL(noMemory()), this, SLOT(sltNoMemory()));

    thread->start();
}

bool RenderArea::event(QEvent *event)
{
    if (QEvent::Gesture == event->type())
        return gestureEvent(static_cast<QGestureEvent*>(event));
    return QWidget::event(event);
}

bool RenderArea::gestureEvent(QGestureEvent *event)
{
    qCDebug(lcExample) << "gestureEvent():" << event;
    if (QGesture *pan = event->gesture(Qt::PanGesture)) {
        qCDebug(lcExample) << "panTriggered()";
    }
    if (QGesture *pinch = event->gesture(Qt::PinchGesture)) {
        qCDebug(lcExample) << "pinchTriggered()";
        pinchTriggered(static_cast<QPinchGesture *>(pinch));
    }
    if (QGesture *tap = event->gesture(Qt::TapGesture)) {
        qCDebug(lcExample) << "tapTriggered()";
    }
    return true;
}

void RenderArea::pinchTriggered(QPinchGesture *gesture)
{
    QPinchGesture::ChangeFlags changeFlags(gesture->changeFlags());
    if (changeFlags & QPinchGesture::ScaleFactorChanged) {
        screenScale = gesture->totalScaleFactor();
        qCDebug(lcExample) << "pinchTriggered(): zoom by" << gesture->scaleFactor() << "->" << screenScale;
    }
    update();
}
#endif
