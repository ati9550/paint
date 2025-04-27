#pragma once

#include <QGraphicsView>
#include <QWidget>
#include <QMouseEvent>

class Tool : public QWidget {
    Q_OBJECT
    
public:
    explicit Tool(QWidget *parent = nullptr, QGraphicsView *currentView = nullptr) : QWidget(parent) {
        view = currentView;
    }
    
    virtual void mousePressEvent(QMouseEvent *event) = 0;
    virtual void mouseMoveEvent(QMouseEvent *event) = 0;
    virtual void mouseReleaseEvent(QMouseEvent *event) = 0;
    
    virtual QWidget* createToolOptionsWidget() = 0;
    
    QGraphicsView *view;
    
signals:
    void done();
};