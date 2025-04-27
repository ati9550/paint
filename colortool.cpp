#include "colortool.h"
#include <QPainter>
#include <QGraphicsPixmapItem>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QBitmap>
#include <QCheckBox>
#include <cmath>
#include <QPixmap>

ColorTool::ColorTool(QWidget *parent, QGraphicsView *currentView, SelectColorButton *colorButton) : Tool(parent) {
    view = currentView;
    button = colorButton;
}

void ColorTool::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        QSize size = parent()->property("canvasSize").value<QSize>();
        QPixmap pixmap(size);
        QPainter painter(&pixmap);
        view->scene()->render(&painter, QRectF(0, 0, size.width(), size.height()));
        painter.end();
        QPointF point = view->mapToScene(event->pos());
        QColor color = pixmap.toImage().pixelColor(point.toPoint());
        color.setAlpha(255);
        QColor brushColor = parent()->setProperty("activeColor", color);
        if (button) button->setColor(color);
    }
}


void ColorTool::mouseMoveEvent(QMouseEvent *event) {
    return;    
}

void ColorTool::mouseReleaseEvent(QMouseEvent *event) {
    return;    
}

QWidget* ColorTool::createToolOptionsWidget() {
    QWidget *optionsWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    
    layout->addWidget(new QLabel("Взять цвет с холста"));
    optionsWidget->setLayout(layout);
    return optionsWidget;
}