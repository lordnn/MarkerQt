/**
 * The MIT License (MIT)
 * Copyright (c) 2017-2018 Kirill Lebedev
**/

#ifndef CORNERGRABBER_H
#define CORNERGRABBER_H

#include <QGraphicsItem>
#include <QPen>

class CornerGrabber : public QGraphicsItem
{
public:
    enum class KMouse { kMouseReleased = 0, kMouseDown, kMouseMoving };
    explicit CornerGrabber(QGraphicsItem *parent = nullptr, int corner = 0);

    int getCorner() const;
    void setMouseState(KMouse);
    KMouse getMouseState() const;

    qreal mouseDownX{}, mouseDownY{};
    QRectF boundingRect() const Q_DECL_OVERRIDE;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) Q_DECL_OVERRIDE;

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) Q_DECL_OVERRIDE;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) Q_DECL_OVERRIDE;

private:
    QColor _outterborderColor;
    QPen _outterborderPen;

    constexpr static qreal _width{8}, _height{8};
    int _corner;
    KMouse _mouseButtonState{KMouse::kMouseReleased};
};

#endif // CORNERGRABBER_H
