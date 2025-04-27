#include "canvas.h"
#include <QScrollBar>

Canvas::Canvas(QObject *parent, Tool **tool) : isPanning(false) {
    currentRotation = 0;
    currentScale = 1.0;
    this->activeToolPtr = tool;
    this->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
    this->setDragMode(ScrollHandDrag);
}

void Canvas::setSliders(QSlider *zoom, QSlider *rotation) {
    this->zoomSlider = zoom;
    this->rotationSlider = rotation;
}


void Canvas::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        isPanning = true;
        panStartX = event->x();
        panStartY = event->y();
        setCursor(Qt::ClosedHandCursor);
    } else {
        if (*activeToolPtr) (*activeToolPtr)->mousePressEvent(event);
        else QGraphicsView::mousePressEvent(event);
    }
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {   
    if (isPanning) {
        int deltaX = event->x() - panStartX;
        int deltaY = event->y() - panStartY;
        this->horizontalScrollBar()->setValue(this->horizontalScrollBar()->value() - deltaX);
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - deltaY);
        panStartX = event->x();
        panStartY = event->y();
    } else {
        if (*activeToolPtr) (*activeToolPtr)->mouseMoveEvent(event);
        else QGraphicsView::mouseMoveEvent(event);
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        isPanning = false;
        setCursor(Qt::ArrowCursor);
    } else {
        if (*activeToolPtr) (*activeToolPtr)->mouseReleaseEvent(event);
        else QGraphicsView::mouseReleaseEvent(event);
    }
}

void Canvas::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        double scaleFactor = 1.15;
        if (event->angleDelta().y() > 0) {
            zoomChanged(this->currentScale * scaleFactor * 100);
        } else {
            if (this->currentScale > 0.1)
                zoomChanged(this->currentScale * (1.0 / scaleFactor) * 100);
        }
        if (zoomSlider != nullptr) zoomSlider->setValue(this->currentScale * 100);
        event->accept();  // Блокируем стандартное событие прокрутки
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void Canvas::zoomChanged(int value) {
    double scaleValue = value / 100.0;
    QTransform transform;
    transform.translate(this->width() / 2, this->height() / 2);
    transform.scale(scaleValue, scaleValue);  // Масштабируем относительно центра
    transform.rotate(currentRotation);  // Применяем текущий поворот
    this->setTransform(transform);
    currentScale = scaleValue;  // Обновляем текущее значение масштаба
}

void Canvas::rotationChanged(int value) {
    QTransform transform;
    transform.translate(this->width() / 2, this->height() / 2);
    transform.scale(currentScale, currentScale);  // Применяем текущий масштаб
    transform.rotate(value);  // Применяем новый поворот
    this->setTransform(transform);
    
    currentRotation = value;  // Обновляем текущее значение поворота
}