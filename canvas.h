#pragma once

#include <QGraphicsView>
#include "tool.h"
#include <QSlider>

class Canvas : public QGraphicsView {
    Q_OBJECT
    
public:
    explicit Canvas(QObject *parent = nullptr, Tool **tool = nullptr);
    void setSliders(QSlider *zoom, QSlider *rotation);
    
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    
private:
    bool isPanning;
    int panStartX, panStartY;
    
    double currentRotation;
    double currentScale;
    
    Tool **activeToolPtr;
    
    QSlider *zoomSlider;
    QSlider *rotationSlider;
    
public slots:
    void zoomChanged(int value);
    void rotationChanged(int value);
};
