/**
 * The MIT License (MIT)
 * Copyright (c) 2017-2018 Kirill Lebedev
**/

#include "cornergrabber.h"

#include <QPainter>

CornerGrabber::CornerGrabber(QGraphicsItem *parent, int corner) : QGraphicsItem(parent), _outterborderColor(Qt::black), _outterborderPen(), _corner(corner) {
    setParentItem(parent);
    _outterborderPen.setWidth(2);
    _outterborderPen.setColor(_outterborderColor);

    this->setAcceptHoverEvents(true);
}

void CornerGrabber::setMouseState(KMouse s) {
    _mouseButtonState = s;
}

CornerGrabber::KMouse CornerGrabber::getMouseState() const {
    return _mouseButtonState;
}

int CornerGrabber::getCorner() const {
    return _corner;
}

void CornerGrabber::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
    _outterborderColor = Qt::black;
    this->update(0, 0, _width, _height);
}

void CornerGrabber::hoverEnterEvent(QGraphicsSceneHoverEvent *) {
    _outterborderColor = Qt::red;
    this->update(0, 0, _width, _height);
}

QRectF CornerGrabber::boundingRect() const {
    return QRectF(0, 0, _width, _height);
}

void CornerGrabber::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    _outterborderPen.setCapStyle(Qt::SquareCap);
    _outterborderPen.setStyle(Qt::SolidLine);
    painter->setPen(_outterborderPen);

    QPointF topLeft(0, 0);
    QPointF bottomRight(_width, _height);

    QRectF rect(topLeft, bottomRight);

    QBrush brush(Qt::SolidPattern);
    brush.setColor(_outterborderColor);
    painter->fillRect(rect, brush);
}
