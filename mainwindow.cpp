#include "mainwindow.h"
#include "brushtool.h"
#include "buckettool.h"
#include "movetool.h"
#include "colortool.h"
#include "canvas.h"
#include "selectcolorbutton.h"
#include <QLabel>
#include <QMouseEvent>
#include <QScrollBar>
#include <QSlider>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QMenuBar>
#include <QFileDialog>
#include <QColorDialog>
#include <QInputDialog>
#include <QIcon>
#include <QFrame>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), activeTool(nullptr), activeToolWidget(nullptr) {
    this->setWindowTitle("Растра");
    
    graphicsView = new Canvas(this, &this->activeTool);
    graphicsScene = new QGraphicsScene(this);
    graphicsView->setScene(graphicsScene);
    graphicsView->setRenderHint(QPainter::Antialiasing);
    graphicsView->setCacheMode(QGraphicsView::CacheBackground);
    graphicsView->setDragMode(QGraphicsView::NoDrag);
    graphicsView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setCentralWidget(graphicsView);
    
    this->setProperty("canvasSize", QSize(800, 600));
    
    setupDockWidgets();
    setupStatusBar();
    setupMenu();
}

void MainWindow::setupDockWidgets() {
    QDockWidget *toolDock = new QDockWidget(tr("Инструменты"), this);
    QWidget *toolWidget = new QWidget(this);
    QVBoxLayout *toolLayout = new QVBoxLayout;
    
    QHBoxLayout* colorLayout = new QHBoxLayout;
    QLabel* colorLabel = new QLabel(" Цвет:");
    colorLayout->addWidget(colorLabel);
    SelectColorButton *colorButton = new SelectColorButton(this);
    colorLayout->addWidget(colorButton);
    
    
    QListWidget *toolList = new QListWidget(this);
    toolList->setIconSize(QSize(48, 48));
    toolList->addItem("Кисть");
    toolList->item(0)->setIcon(QIcon::fromTheme("tool_brush"));
    toolList->addItem("Ластик");
    toolList->item(1)->setIcon(QIcon::fromTheme("tool_eraser"));    
    toolList->addItem("Заливка");
    toolList->item(2)->setIcon(QIcon::fromTheme("tool_flood_fill"));
    toolList->addItem("Сдвиг");
    toolList->item(3)->setIcon(QIcon::fromTheme("transform-move"));
    toolList->addItem("Масштаб");
    toolList->item(4)->setIcon(QIcon::fromTheme("transform-scale"));
    toolList->addItem("Пипетка");
    toolList->item(5)->setIcon(QIcon::fromTheme("tool_color_picker"));
    
    
    toolLayout->addWidget(toolList);
    
    toolLayout->addLayout(colorLayout);
    
    
    activeToolWidget = new QLabel("Выберите инструмент");
    activeToolLayout = new QVBoxLayout();
    activeToolLayout->addWidget(activeToolWidget);
    QFrame *activeToolFrame = new QFrame();
    activeToolFrame->setLayout(activeToolLayout);
    activeToolFrame->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    toolLayout->addWidget(activeToolFrame);
    
    tools.append(new BrushTool(this, graphicsView, false));
    tools.append(new BrushTool(this, graphicsView, true));
    tools.append(new BucketTool(this, graphicsView));
    tools.append(new MoveTool(this, graphicsView, false));
    tools.append(new MoveTool(this, graphicsView, true));
    tools.append(new ColorTool(this, graphicsView, colorButton));
    
    connect(toolList, &QListWidget::currentRowChanged, this, [this](int currentRow) {
        activeTool = tools[currentRow];
        
        connect(activeTool, &Tool::done, this, [=]() {
            if (this->property("activeLayer").value<QGraphicsItem*>() == this->property("maskLayer").value<QGraphicsItem*>())
                layerManager->historySave(QVector<int>({0}));
            else
                layerManager->historySave(QVector<int>({0, (int)this->property("activeLayer").value<QGraphicsItem*>()->zValue() / 2}));                
            layerManager->updateThumbnails();    
        });

        if (this->activeToolWidget) {
            this->activeToolLayout->removeWidget(activeToolWidget);
            delete activeToolWidget;
        }
        this->activeToolWidget = activeTool->createToolOptionsWidget();
        this->activeToolLayout->addWidget(activeToolWidget);    
    });
        
    toolWidget->setLayout(toolLayout);
    toolDock->setWidget(toolWidget);
    addDockWidget(Qt::LeftDockWidgetArea, toolDock);
    
    // Создаем док для слоев
    layerManager = new LayerManager(this, graphicsScene);
    QDockWidget *layerDock = new QDockWidget(tr("Слои"), this);
    
    layerDock->setWidget(layerManager);
    addDockWidget(Qt::RightDockWidgetArea, layerDock);
    
    toolList->setCurrentRow(0);
}

void MainWindow::setupStatusBar() {
    QStatusBar *statusBar = new QStatusBar(this);
    
    
    QSlider *zoomSlider = new QSlider(Qt::Horizontal, this);
    QSlider *rotationSlider = new QSlider(Qt::Horizontal, this);
    
    zoomSlider->setRange(10, 800);
    zoomSlider->setValue(100);
    zoomSlider->setTickPosition(QSlider::TicksBothSides);
    zoomSlider->setTickInterval(100);
    zoomSlider->setSingleStep(10);
    rotationSlider->setRange(-180, 180);
    rotationSlider->setTickPosition(QSlider::TicksBothSides);
    rotationSlider->setTickInterval(45);
    rotationSlider->setSingleStep(15);
    
    statusBar->addPermanentWidget(new QLabel("Масштаб"));    
    statusBar->addPermanentWidget(zoomSlider);
    statusBar->addPermanentWidget(new QLabel("Поворот"));    
    statusBar->addPermanentWidget(rotationSlider);
    setStatusBar(statusBar);
    
    connect(zoomSlider, &QSlider::valueChanged, graphicsView, &Canvas::zoomChanged);
    connect(rotationSlider, &QSlider::valueChanged, graphicsView, &Canvas::rotationChanged);
    
    graphicsView->setSliders(zoomSlider, rotationSlider);
}

void MainWindow::setupMenu() {
    QMenu *fileMenu = menuBar()->addMenu(tr("Файл"));
    QAction *addLayerAction = new QAction(tr("Добавить слой"), this);
    QAction *exportAction = new QAction(tr("Экспорт"), this);
    QAction *saveAction = new QAction(tr("Сохранить"), this);
    QAction *openAction = new QAction(tr("Открыть"), this);
    
    saveAction->setShortcut(QKeySequence("Ctrl+S"));
    openAction->setShortcut(QKeySequence("Ctrl+O"));
        
    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(exportAction);
    fileMenu->addAction(addLayerAction);
    
    
    connect(addLayerAction, &QAction::triggered, this, &MainWindow::addLayerFromFile);
    connect(exportAction, &QAction::triggered, this, &MainWindow::saveImage);
    connect(saveAction, &QAction::triggered, this, [=](){layerManager->saveLayers();});
    connect(openAction, &QAction::triggered, this, [=](){layerManager->openLayers();});
    
    QMenu *editMenu = menuBar()->addMenu(tr("Правка"));
    QAction *undoAction = new QAction(tr("Отменить"), this);
    QAction *redoAction = new QAction(tr("Повторить"), this);
    
    undoAction->setShortcut(QKeySequence("Ctrl+Z"));
    redoAction->setShortcut(QKeySequence("Ctrl+Shift+Z"));
    
    editMenu->addAction(undoAction);
    editMenu->addAction(redoAction);
    
    connect(undoAction, &QAction::triggered, this, [=](){layerManager->historyUndo();});
    connect(redoAction, &QAction::triggered, this, [=](){layerManager->historyRepeat();});
    
    QMenu *canvasMenu = menuBar()->addMenu(tr("Холст"));
    QAction *scaleCanvasAction = new QAction(tr("Растянуть"), this);
    connect(scaleCanvasAction, &QAction::triggered, this, [this]() {
        bool ok;
        int width = QInputDialog::getInt(this, tr("Растянуть"), tr("Новая ширина:"),
                                         this->property("canvasSize").value<QSize>().width(), 1, 10000, 1, &ok);
        if (ok) {
            int height = QInputDialog::getInt(this, tr("Растянуть"), tr("Новая высота:"),
                                              this->property("canvasSize").value<QSize>().height(), 1, 10000, 1, &ok);
            if (ok) {
                this->setProperty("canvasSize", QSize(width, height));
                layerManager->resize(width, height, width, height);
            }
        }
    });
    canvasMenu->addAction(scaleCanvasAction);
    
    QAction *cropCanvasAction = new QAction(tr("Обрезать"), this);
    connect(cropCanvasAction, &QAction::triggered, this, [this]() {
        bool ok;
        int width = QInputDialog::getInt(this, tr("Обрезать"), tr("Новая ширина:"),
                                         this->property("canvasSize").value<QSize>().width(), 1, 10000, 1, &ok);
        if (ok) {
            int height = QInputDialog::getInt(this, tr("Обрезать"), tr("Новая высота:"),
                                              this->property("canvasSize").value<QSize>().height(), 1, 10000, 1, &ok);
            if (ok) {
                layerManager->resize(this->property("canvasSize").value<QSize>().width(),
                             this->property("canvasSize").value<QSize>().height(), width, height);
                this->setProperty("canvasSize", QSize(width, height));
            }
        }
    });
    canvasMenu->addAction(cropCanvasAction);
    
    QMenu *effectMenu = menuBar()->addMenu(tr("Эффекты"));
    QAction *invertAction = new QAction(tr("Инвертировать"), this);
    QAction *brightnessContrasAction = new QAction(tr("Яркость / контраст"), this);
    
    connect(invertAction, &QAction::triggered, this, [=](){layerManager->invertLayer();});
    effectMenu->addAction(invertAction);
    effectMenu->addAction(brightnessContrasAction);
    
    connect(brightnessContrasAction, &QAction::triggered, this, [=]() {
        bool ok = false;
        int brightness = QInputDialog::getInt(this, tr("Яркость"), tr("Введите значение (-255 до 255):"), 0, -255, 255, 1, &ok);
        if (!ok) return;
        double contrast = QInputDialog::getDouble(this, tr("Контраст"), tr("Введите значение (0.0 до 5.0):"), 1.0, 0.0, 5.0, 2, &ok);
        if (!ok) return;
        layerManager->adjustBrightnessContrast(brightness, contrast);
    });
}

void MainWindow::addLayerFromFile() {
    QString filePath = QFileDialog::getOpenFileName(this, tr("Выберите изображение"), "", tr("Images (*.png *.jpg)"));
    if (!filePath.isEmpty()) {
        layerManager->addLayerFromFile(filePath);
    }
}

void MainWindow::saveImage() {
    QString filePath = QFileDialog::getSaveFileName(this, tr("Сохранить изображение"), "", tr("PNG Files (*.png);;JPEG Files (*.jpg)"));
    if (!filePath.isEmpty()) {
        QPixmap pixmap(this->property("canvasSize").value<QSize>());
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        graphicsScene->render(&painter);
        pixmap.save(filePath);
    }
}
