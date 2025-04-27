#include "movetool.h"
#include <QPainter>
#include <QGraphicsPixmapItem>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QRegion>
#include <QBitmap>
#include <cmath>

MoveTool::MoveTool(QWidget *parent, QGraphicsView *currentView, bool isScaler)
    : Tool(parent), currentLayer(nullptr) {
    view = currentView;
    activeArea = nullptr;
    modeScaler = isScaler;
}

void MoveTool::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        currentLayer = dynamic_cast<QGraphicsPixmapItem*>(parent()->property("activeLayer").value<QGraphicsItem*>());
        maskLayer = dynamic_cast<QGraphicsPixmapItem*>(parent()->property("maskLayer").value<QGraphicsItem*>());
        QSize layerSize = parent()->property("canvasSize").value<QSize>();
                
        if (currentLayer && maskLayer && layerSize.isValid()) {
            lastPoint = view->mapToScene(event->pos());
            activeArea = new QPixmap(layerSize);
            activeArea->fill(Qt::transparent);
            QPainter painter(activeArea);
            painter.setClipRegion(QRegion(maskLayer->pixmap().mask()));
            painter.drawPixmap(currentLayer->boundingRect().toRect(), currentLayer->pixmap());
            painter.end();
            
            QPixmap pixmap = currentLayer->pixmap();
            QPainter painter2(&pixmap);
            painter2.setCompositionMode(QPainter::CompositionMode_Clear);
            painter2.setClipRegion(QRegion(maskLayer->pixmap().mask()));            
            painter2.drawPixmap(currentLayer->boundingRect().toRect(), *activeArea);
            painter2.end();
            currentLayer->setPixmap(pixmap);
        }
        else qDebug("there is no layer");
    }
}

void MoveTool::mouseMoveEvent(QMouseEvent *event) {
    return;
}

void MoveTool::mouseReleaseEvent(QMouseEvent *event) {
    if (activeArea != nullptr && !activeArea->isNull()) {
        QPointF newPoint = view->mapToScene(event->pos());
        
        QPixmap pixmap = currentLayer->pixmap();
        if (pixmap.isNull()) qDebug("the pixmap is null");
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        
        QPixmap pixmap2(maskLayer->pixmap().width(), maskLayer->pixmap().height());
        pixmap2.fill(Qt::black);
        QPainter painter2(&pixmap2);
        painter2.setCompositionMode(QPainter::CompositionMode_Source);
        painter2.setRenderHint(QPainter::SmoothPixmapTransform);
        
        if (modeScaler) {
            int scaleScale = pow(currentLayer->boundingRect().width() +
                                 currentLayer->boundingRect().height(), 2) / 10000;
            
            double scaleX = 1.0 + (newPoint.x() - lastPoint.x()) / scaleScale;
            double scaleY = 1.0 + (newPoint.y() - lastPoint.y()) / scaleScale;
            
            double newWidth = activeArea->width() * scaleX;
            double newHeight = activeArea->height() * scaleY;
            
            // Смещение относительно точки lastPoint
            double originX = lastPoint.x() - newWidth * (lastPoint.x() / activeArea->width());
            double originY = lastPoint.y() - newHeight * (lastPoint.y() / activeArea->height());
            painter.drawPixmap(originX, originX, newWidth, newHeight, *activeArea);
            if (maskLayer != currentLayer)
                painter2.drawPixmap(originX, originX, newWidth, newHeight, pixmap2);
        }
        else {
            painter.drawPixmap(newPoint.toPoint().x() - lastPoint.toPoint().x(),
                               newPoint.toPoint().y() - lastPoint.toPoint().y(),
                               activeArea->width(), activeArea->height(), *activeArea);
            if (maskLayer != currentLayer)
                painter2.drawPixmap(newPoint.toPoint().x() - lastPoint.toPoint().x(),
                                    newPoint.toPoint().y() - lastPoint.toPoint().y(),
                                    activeArea->width(), activeArea->height(), maskLayer->pixmap());
        }
        currentLayer->setPixmap(pixmap);
        if (maskLayer != currentLayer) maskLayer->setPixmap(pixmap2);
        delete activeArea;
        activeArea = nullptr;
        emit done();                
    }
}

QWidget* MoveTool::createToolOptionsWidget() {
    QWidget *optionsWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    
    if (modeScaler) layout->addWidget(new QLabel("Тяните чтобы масштабировать\n"
                                                 "В правый нижний угол - увеличить\n"
                                                 "В левый верхний - уменьшить"));
    else layout->addWidget(new QLabel("Тяните чтобы переместить"));
    
    optionsWidget->setLayout(layout);
    return optionsWidget;
}