#include "buckettool.h"
#include <QPainter>
#include <QGraphicsPixmapItem>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QBitmap>
#include <QCheckBox>
#include <cmath>

BucketTool::BucketTool(QWidget *parent, QGraphicsView *currentView)
    : Tool(parent), brushColor(Qt::black), currentLayer(nullptr), modeEraser(false) {
    view = currentView;
    brushOpacity = 255;
}

void BucketTool::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        currentLayer = dynamic_cast<QGraphicsPixmapItem*>(parent()->property("activeLayer").value<QGraphicsItem*>());
        maskLayer = dynamic_cast<QGraphicsPixmapItem*>(parent()->property("maskLayer").value<QGraphicsItem*>());
        
        brushColor = parent()->property("activeColor").value<QColor>();
        brushColor.setAlpha(brushOpacity);
        if (currentLayer) {
            lastPoint = view->mapToScene(event->pos());
            FloodFill(lastPoint.toPoint(), brushColor);
            emit done();            
        }
        else qDebug("there is no layer");
    }
}


void BucketTool::mouseMoveEvent(QMouseEvent *event) {
    return;    
}

void BucketTool::mouseReleaseEvent(QMouseEvent *event) {
    return;    
}

QWidget* BucketTool::createToolOptionsWidget() {
    QWidget *optionsWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    
    QCheckBox *isEraser = new QCheckBox("Стирать");
    isEraser->setChecked(modeEraser);
    connect(isEraser, &QCheckBox::toggled, this, [=](){modeEraser = isEraser->isChecked();});
    layout->addWidget(isEraser);
    
    layout->addWidget(new QLabel("Нажмите чтобы залить"));
    optionsWidget->setLayout(layout);
    return optionsWidget;
}


void BucketTool::setBrushOpacity(int value) {
    brushOpacity = value;
}

std::deque<QPoint> BucketTool::GetPoints(QImage& image, QPoint seed) {

    if (image.format() != QImage::Format_ARGB32_Premultiplied) {
        throw std::runtime_error("Работает только с QImage::Format_ARGB32_Premultiplied");
    }
    
    const int width = image.width();
    const int height = image.height();
    
    QRgb* bits = reinterpret_cast<QRgb*>(image.bits());
    
    const auto getPixel = [&](int x, int y) {
        return bits[(y * width) + x];
    };
    
    const QRgb oldRgba = getPixel(seed.x(), seed.y());
    
    std::vector<unsigned char> processedAlready(width * height);
    
    std::deque<QPoint> backlog = { seed };
    std::deque<QPoint> points;
    
    while (!backlog.empty()) {
        const QPoint& point = backlog.front();
        const int x = point.x();
        const int y = point.y();
        if (x >= 0 && y >= 0 && x < width && y < height) {
            const QRgb rgba = getPixel(x, y);
            if (rgba == oldRgba) {
                unsigned char& isProcessedAlready = processedAlready[(y * width) + x];
                if (!isProcessedAlready) {
                    isProcessedAlready = true;
                    points.push_back(point);
                    backlog.push_back(QPoint(x - 1, y));
                    backlog.push_back(QPoint(x, y - 1));
                    backlog.push_back(QPoint(x + 1, y));
                    backlog.push_back(QPoint(x, y + 1));
                }
            }
        }
        backlog.pop_front();
    }
    
    return points;
}

void BucketTool::FloodFill(QPoint seed, QColor newColor) {
    QPixmap pixmap = currentLayer->pixmap();
    QImage image = pixmap.toImage();
    
    if (image.format() != QImage::Format_ARGB32_Premultiplied) {
        image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }
    
    const auto points = GetPoints(image, seed);
    
    
    QPainter painter(&pixmap);
    painter.setPen(newColor);
    painter.setBrush(newColor);
    
    if (maskLayer && maskLayer != currentLayer) painter.setClipRegion(QRegion(maskLayer->pixmap().mask()));
    
    if (modeEraser) painter.setCompositionMode(QPainter::CompositionMode_Clear);
    else painter.setCompositionMode(QPainter::CompositionMode_Source);
    
    for (const QPoint& point : points) {
        painter.drawPoint(point);
    }
    currentLayer->setPixmap(pixmap);
}
