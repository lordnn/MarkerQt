/**
 * The MIT License (MIT)
 * Copyright (c) 2017-2018 Kirill Lebedev
**/

#ifndef WORKER_H
#define WORKER_H

#include "base.h"
#include <QObject>
#include <QImage>

//#include <dlib/image_processing/frontal_face_detector.h>

class TWorker : public QObject
{
    Q_OBJECT

public:
    enum class workerType {
        wtFaceDetector = 1,
        wtLBFRDetector = 2

    };

    TWorker(workerType type);
    void setData(const QImage &img);
    void setRect(const QRect &rect);

public slots:
    void process();

signals:
    void finished();
    void noMemory();
    void completeFaceDetector(CRectArray &frects);
    void completeLBFRDetector(CPointFArray &pts);

private:
    QImage _image;
    QRect _rect;
    workerType _type;
};

#endif // WORKER_H
