#pragma once

#include "tool.h"
#include <QColor>
#include <QPoint>
#include <QPainter>
#include <QRegion>

class QGraphicsPixmapItem;

class BrushTool : public Tool {
    Q_OBJECT
    
public:
    explicit BrushTool(QWidget *parent = nullptr, QGraphicsView *currentView = nullptr, bool isEraser = false);
    
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    
    QWidget* createToolOptionsWidget() override;
    
private:
    int brushThickness;
    int brushOpacity;
    QColor brushColor;
    QGraphicsPixmapItem *currentLayer;
    QGraphicsPixmapItem *maskLayer;
    QPointF lastPoint;
    int time;
    QRegion mask;
    bool modeEraser;
    
    void setBrushThickness(int value);
    void setBrushOpacity(int value);
};