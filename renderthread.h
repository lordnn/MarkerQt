/**
 * The MIT License (MIT)
 * Copyright (c) 2017-2018 Kirill Lebedev
**/

#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QMutex>
#include <QImage>
#include <QPointF>
#include <QSize>
#include <QThread>
#include <QWaitCondition>
#include <atomic>

class RenderThread : public QThread
{
    Q_OBJECT
public:
    RenderThread(QObject *parent = 0);
    ~RenderThread();

    void render(QPointF scrCenter, qreal scaleFactor,
                QSize resultSize, const QImage &im, const std::vector<QRect> &frects, const std::vector<QPointF> &pts);

signals:
    void renderedImage(const QImage &image, QPointF center, qreal scaleFactor);

protected:
    void run() Q_DECL_OVERRIDE;

private:
    QMutex mutex;
    QWaitCondition condition;
    QPointF _scrCenter;
    qreal _scaleFactor;
    QSize _resultSize;
    QImage _image;
    std::vector<QRect> _frects;
    std::vector<QPointF> _pts;
    std::atomic<bool> restart;
    bool abort;
};

#endif
