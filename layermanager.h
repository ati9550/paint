#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QVector>
#include <QStack>
#include <QImage>

class LayerManager : public QWidget {
    Q_OBJECT
    
public:
    explicit LayerManager(QWidget *parent = nullptr, QGraphicsScene *newScene = nullptr);
    
    void addLayer();    
    void addLayer(QGraphicsItem *layer, QString layerName);
    void addLayerFromFile(const QString &filePath);
    void removeLayer();
    void removeLayer(int index);
    
    void setScene(QGraphicsScene* newScene);
    void updateThumbnails();
    void resize(int imageWidth, int imageHeight, int canvasWidth, int canvasHeight);
    
    void saveLayers();
    void openLayers();
    
    void invertLayer();
    void adjustBrightnessContrast(int brightness, double contrast);
    
    // Операции истории
    void historyClear();
    void historySave(QVector<int> changedLayers);
    void historySave();
    void historyUndo();
    void historyRepeat();
    
    void updateLayers(const QVector<QPair<QString, QImage>> &state);
    
private:
    QListWidget *layerList;
    QGraphicsScene *scene;
    QVector<QGraphicsItem*> layerItems;
    
    QStack<QVector<QPair<QString, QByteArray>>> undoStack;
    QStack<QVector<QPair<QString, QByteArray>>> redoStack;
    
    QVector<QPair<QString, QImage>> previousState;
    
    QVector<QPair<QString, QImage>> getCurrentState();
    QVector<QPair<QString, QImage>> getCurrentState(QVector<int> changedLayers);    
    QVector<QPair<QString, QByteArray>> calculateDiff(const QVector<QPair<QString, QImage>> &oldState, const QVector<QPair<QString, QImage>> &newState);
    QVector<QPair<QString, QImage>> applyDiff(const QVector<QPair<QString, QByteArray>> &diff, const QVector<QPair<QString, QImage>> &currentState);
};