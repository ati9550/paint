#include "brushtool.h"
#include <QPainter>
#include <QGraphicsPixmapItem>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QRegion>
#include <QBitmap>
#include <cmath>

BrushTool::BrushTool(QWidget *parent, QGraphicsView *currentView, bool isEraser)
    : Tool(parent), brushThickness(5), brushOpacity(255), brushColor(Qt::black), currentLayer(nullptr) {
    view = currentView;
    modeEraser = isEraser;
}

void BrushTool::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        currentLayer = dynamic_cast<QGraphicsPixmapItem*>(parent()->property("activeLayer").value<QGraphicsItem*>());
        maskLayer = dynamic_cast<QGraphicsPixmapItem*>(parent()->property("maskLayer").value<QGraphicsItem*>());
        if (!maskLayer) qDebug("there is no layer");

        brushColor = parent()->property("activeColor").value<QColor>();

        if (currentLayer) {
            lastPoint = view->mapToScene(event->pos());
            time = 0;
            if (maskLayer && maskLayer != currentLayer) mask = QRegion(maskLayer->pixmap().mask());
            
        }
        else qDebug("there is no layer");
    }
}

void BrushTool::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() == Qt::LeftButton && currentLayer) {
        QPointF newPoint = view->mapToScene(event->pos());


        qreal distance = sqrt(pow((lastPoint.x() - newPoint.x()), 2) +
                              pow((lastPoint.y() - newPoint.y()), 2));

        qreal density =  sqrt(pow((currentLayer->boundingRect().width()), 2) +
                             pow((currentLayer->boundingRect().height()), 2));

        qreal speed = distance < 1 ? 1.0 : sqrt(distance * 300.0) / 15.0;

        newPoint.setX(lastPoint.x() + (newPoint.x() - lastPoint.x()) / speed);
        newPoint.setY(lastPoint.y() + (newPoint.y() - lastPoint.y()) / speed);

        if (distance < density / 10000.0 && time < 16) {
            time++;
            return;
        }

        time = 0;

        qreal thickness = brushThickness / (1.0 + pow(speed * 2.0, 2) / 1000.0);

        distance = sqrt(pow((lastPoint.x() - newPoint.x()), 2) +
                        pow((lastPoint.y() - newPoint.y()), 2));
            
        QPixmap pixmap = currentLayer->pixmap();
        if (pixmap.isNull()) qDebug("the pixmap is null");
        QPainter painter(&pixmap);
        QPen pen(brushColor);
        pen.setWidth(thickness);
        pen.setColor(brushColor);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        painter.setRenderHint(QPainter::Antialiasing);
        
        if (maskLayer && maskLayer != currentLayer) painter.setClipRegion(mask);
        if (modeEraser) painter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        
        painter.setOpacity(brushOpacity * 0.5 / 255.0 * distance / sqrt(thickness));
        painter.drawLine(lastPoint, newPoint);
        lastPoint = newPoint;
        currentLayer->setPixmap(pixmap);
        painter.end();
    }
}

void BrushTool::mouseReleaseEvent(QMouseEvent *event) {
    emit done();
    return;
}

QWidget* BrushTool::createToolOptionsWidget() {
    QWidget *optionsWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    
    layout->addWidget(new QLabel("Толщина"));
    // Слайдер для толщины кисти
    QSlider *thicknessSlider = new QSlider(Qt::Horizontal);
    thicknessSlider->setRange(1, 240);
    thicknessSlider->setValue(brushThickness);
    connect(thicknessSlider, &QSlider::valueChanged, this, &BrushTool::setBrushThickness);
    layout->addWidget(thicknessSlider);
    
    layout->addWidget(new QLabel("Прозрачность"));    
    // Слайдер для прозрачности кисти
    QSlider *opacitySlider = new QSlider(Qt::Horizontal);
    opacitySlider->setRange(0, 255);
    opacitySlider->setValue(brushOpacity);
    connect(opacitySlider, &QSlider::valueChanged, this, &BrushTool::setBrushOpacity);
    layout->addWidget(opacitySlider);
    
    optionsWidget->setLayout(layout);
    return optionsWidget;
}

void BrushTool::setBrushThickness(int value) {
    brushThickness = value;
}

void BrushTool::setBrushOpacity(int value) {
    brushOpacity = value;
}
