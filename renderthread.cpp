/**
 * The MIT License (MIT)
 * Copyright (c) 2017-2018 Kirill Lebedev
**/

#include "renderthread.h"
#include <QtGui>
#include <array>
#include <cmath>

#define StepX 20
#define StepY 10

class mapperC {
public:
    constexpr mapperC(int centerTo, qreal centerFrom, qreal scale) : _centerTo(centerTo), _centerFrom(centerFrom), _scale(scale)
    { }
    constexpr qreal mapTo(int v) const {
        return _centerTo + (v - _centerFrom) / _scale;
    }
private:
    int _centerTo;
    qreal _centerFrom, _scale;
};

class mapperPt {
public:
    constexpr mapperPt(const QPoint &centerTo, const QPointF &centerFrom, const QPointF &scale) : _centerTo(centerTo), _centerFrom(centerFrom), _scale(scale)
    { }
    QPoint mapTo(const QPointF &v) const {
        return QPoint(std::round(_centerTo.x() + (v.x() - _centerFrom.x()) / _scale.x()), std::round(_centerTo.y() + (v.y() - _centerFrom.y()) / _scale.y()));
    }
    QRectF mapTo(const QRect &v) const {
        return QRectF(mapTo(v.topLeft()), mapTo(v.bottomRight()));
    }
private:
    QPoint _centerTo;
    QPointF _centerFrom, _scale;
};

RenderThread::RenderThread(QObject *parent)
    : QThread(parent), restart(ATOMIC_VAR_INIT(false)), abort(false)
{
}

RenderThread::~RenderThread()
{
	mutex.lock();
	abort = true;
	condition.wakeOne();
	mutex.unlock();

	wait();
}

void RenderThread::render(QPointF scrCenter, qreal scaleFactor,
                          QSize resultSize, const QImage &image, const std::vector<QRect> &frects, const std::vector<QPointF> &pts)
{
	QMutexLocker locker(&mutex);
    //std::cout << "render: " << std::internal <<std::hex << std::setw(16) << std::setfill('0') << image.constBits() << std::endl;
	this->_scrCenter = scrCenter;
	this->_scaleFactor = scaleFactor;
	this->_resultSize = resultSize;
    this->_image = image;
    this->_frects = frects;
    this->_pts = pts;

	if (!isRunning()) {
		start(LowPriority);
	} else {
		restart.store(true, std::memory_order_relaxed);
		condition.wakeOne();
	}
}

void RenderThread::run()
{
    forever {
        mutex.lock();
        QSize resultSize = this->_resultSize;
        qreal scaleFactor = this->_scaleFactor;
        QPointF scrCenter = this->_scrCenter;
        qreal centerX = this->_scrCenter.x();
        qreal centerY = this->_scrCenter.y();
        QImage image = this->_image;
        std::vector<QRect> frects = this->_frects;
        std::vector<QPointF> pts = this->_pts;
        mutex.unlock();

        int halfWidth = resultSize.width() / 2;
        int halfHeight = resultSize.height() / 2;
        QImage imageRes(resultSize, QImage::Format_RGB32);

        if (abort)
            return;

        QPainter painter(&imageRes);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(0, 0, resultSize.width(), resultSize.height(), Qt::white);

        painter.setPen(Qt::black);
        const QRectF rectImg(halfWidth - centerX / scaleFactor, halfHeight - centerY / scaleFactor, image.width() / scaleFactor, image.height() / scaleFactor);
        const QRectF rectDst(QRectF(imageRes.rect()).intersected(rectImg));
        //QRectF rectDst(halfWidth - dstW / 2.f, halfHeight - dstH / 2.f, dstW, dstH);
        if (!rectDst.isEmpty())
        {
            const QRectF rectSrc((rectDst.left() - rectImg.left()) * scaleFactor, (rectDst.top() - rectImg.top()) * scaleFactor, rectDst.width() * scaleFactor, rectDst.height() * scaleFactor);
            painter.drawImage(rectDst, image, rectSrc);
        }

        if (!frects.empty()) {
            const mapperPt mapper(QPoint(halfWidth, halfHeight), scrCenter, QPointF(scaleFactor, scaleFactor));
            QPen oldPen = painter.pen();
            painter.setPen(QPen(Qt::red, 2));
            std::for_each(frects.cbegin(), frects.cend(), [&mapper, &painter](const auto &e){ painter.drawRect(mapper.mapTo(e)); });
            /*const size_t count(frects.size());
            std::array<QPointF, 5> points;
            for (size_t i(0); i < count; ++i) {
                points[0] = mapper.mapTo(frects[i].topLeft()); // QPoint(halfWidth + (-centerX + frects[i].left()) / scaleFactor, halfHeight + (-centerY + frects[i].top()) / scaleFactor);
                points[1] = mapper.mapTo(frects[i].topRight()); // QPoint(halfWidth + (-centerX + frects[i].right()) / scaleFactor, halfHeight + (-centerY + frects[i].top()) / scaleFactor);
                points[2] = mapper.mapTo(frects[i].bottomRight()); // QPoint(halfWidth + (-centerX + frects[i].right()) / scaleFactor, halfHeight + (-centerY + frects[i].bottom()) / scaleFactor);
                points[3] = mapper.mapTo(frects[i].bottomLeft()); // QPoint(halfWidth + (-centerX + frects[i].left()) / scaleFactor, halfHeight + (-centerY + frects[i].bottom()) / scaleFactor);
                points[4] = points[0];
                painter.drawPolyline(points.data(), points.size());
            }*/
            painter.setPen(oldPen);
        }
        if (!pts.empty()) {
            const mapperPt mapper(QPoint(halfWidth, halfHeight), scrCenter, QPointF(scaleFactor, scaleFactor));
            for (size_t i(0); i < pts.size(); ++i) {
                const auto ptDraw = mapper.mapTo(QPointF(pts[i].x(), pts[i].y()));
                painter.drawLine(ptDraw - QPoint(2, 2), ptDraw + QPoint(1, 1));
                painter.drawLine(ptDraw + QPoint(-2, 2), ptDraw + QPoint(1, -1));
            }
        }

        if (!restart.load(std::memory_order_relaxed)) {
            emit renderedImage(imageRes, scrCenter, scaleFactor);
        }
        if (abort)
            return;
        mutex.lock();
        if (!restart.load(std::memory_order_relaxed))
            condition.wait(&mutex);
        restart.store(false, std::memory_order_relaxed);
        mutex.unlock();
    }
}
