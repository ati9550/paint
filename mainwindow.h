#pragma once

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QVBoxLayout>
#include "canvas.h"
#include "tool.h"
#include "layermanager.h"
#include <QVector>

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    
private:
    Canvas *graphicsView;
    QGraphicsScene *graphicsScene;
    QVector<Tool*> tools;
    Tool *activeTool;
    QWidget *activeToolWidget;
    QVBoxLayout *activeToolLayout;
    
    LayerManager *layerManager;
    
    void setupDockWidgets();
    void setupStatusBar();
    void setupMenu();
    
private slots:   
    void addLayerFromFile();
    void saveImage();
};