#pragma once

#include "tool.h"
#include <QColor>
#include <QPoint>

class QGraphicsPixmapItem;

class MoveTool : public Tool {
    Q_OBJECT
    
public:
    explicit MoveTool(QWidget *parent = nullptr, QGraphicsView *currentView = nullptr, bool isScaler = false);
    
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    
    QWidget* createToolOptionsWidget() override;
    
private:
    QGraphicsPixmapItem *currentLayer;
    QGraphicsPixmapItem *maskLayer;
    QPointF lastPoint;
    QPixmap *activeArea;
    bool modeScaler;
};