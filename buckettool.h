#pragma once

#include "tool.h"
#include <QColor>
#include <QPoint>
#include <deque>
#include <unordered_set>
#include <stdexcept>

class QGraphicsPixmapItem;

class BucketTool : public Tool {
    Q_OBJECT
    
public:
    explicit BucketTool(QWidget *parent = nullptr, QGraphicsView *currentView = nullptr);
    
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;    
    void mouseReleaseEvent(QMouseEvent *event) override;
    
    QWidget* createToolOptionsWidget() override;
    
private:
    QColor brushColor;
    int brushOpacity;
    QGraphicsPixmapItem *currentLayer;
    QGraphicsPixmapItem *maskLayer;
    QPointF lastPoint;
    bool modeEraser;    
    
    void setBrushOpacity(int value);
    
    
    std::deque<QPoint> GetPoints(QImage& image, QPoint seed);    
    void FloodFill(QPoint seed, QColor newColor);
};