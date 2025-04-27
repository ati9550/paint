#pragma once

#include "tool.h"
#include <QColor>
#include <QPoint>
#include "selectcolorbutton.h"

class QGraphicsPixmapItem;

class ColorTool : public Tool {
    Q_OBJECT
    
public:
    explicit ColorTool(QWidget *parent = nullptr, QGraphicsView *currentView = nullptr, SelectColorButton *colorButton = nullptr);
    
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;    
    void mouseReleaseEvent(QMouseEvent *event) override;
    
    QWidget* createToolOptionsWidget() override;
    
private:
    SelectColorButton* button;
};