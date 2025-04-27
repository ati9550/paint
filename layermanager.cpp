#include "layermanager.h"
#include <QListWidgetItem>
#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QMetaType>
#include <QListWidget>
#include <QPainter>
#include <QFileDialog>
#include <QInputDialog>
#include <QDir>
#include <QRegularExpression>
#include <QBuffer>

LayerManager::LayerManager(QWidget *parent, QGraphicsScene* newScene) : QWidget(parent) {
    scene = newScene;
    QVBoxLayout *layout = new QVBoxLayout(this);
    // Список слоев
    layerList = new QListWidget(this);
    layerList->setIconSize(QSize(48, 48));
    layerList->setUniformItemSizes(true);
    
    layerList->setDragDropMode(QAbstractItemView::InternalMove);
    layout->addWidget(layerList);
    
    connect(layerList->model(), &QAbstractItemModel::rowsMoved, this, [this](const QModelIndex &parent, int start,
                                                                             int end, const QModelIndex &destination, int row) {
        Q_UNUSED(parent);
        Q_UNUSED(destination);
        // Изменяем Z-значение для всех слоёв
        if (end < row) row--;
        QVector<QPair<QString, QImage>> currentState = getCurrentState({end, row});
        for (int i = 0; i < currentState.size(); i++) {
            if (previousState.size() <= i) previousState.append(qMakePair(currentState[i].first, currentState[i].second));
            else previousState[i].second = currentState[i].second;
        }
        layerItems.move(end, row);
        
        for (int i = 0; i < layerItems.size(); i++) {
            layerItems[i]->setZValue(i * 2);
        }
        
        updateThumbnails();
        historySave(QVector<int>({end, row}));
    });
    
    connect(layerList, &QListWidget::currentRowChanged, this, [this](int currentRow) {
        QGraphicsItem *item = nullptr;
        if (layerItems.size() <= currentRow) currentRow = layerItems.size() - 1;
        if (layerItems.size() > 0) item = layerItems[currentRow];
        
        if (this->parent()->parent())
            this->parent()->parent()->setProperty("activeLayer", QVariant::fromValue(item));
        else
            this->parent()->setProperty("activeLayer", QVariant::fromValue(item));
        updateThumbnails();
    });
    
    QVBoxLayout *panelLayout = new QVBoxLayout();
    QFrame *panelFrame = new QFrame();
    panelFrame->setLayout(panelLayout);
    panelFrame->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    layout->addWidget(panelFrame);
    
    // Кнопки управления слоями
    QPushButton *addLayerButton = new QPushButton(tr("Добавить"), this);
    QPushButton *removeLayerButton = new QPushButton(tr("Удалить"), this);
    panelLayout->addWidget(addLayerButton);
    panelLayout->addWidget(removeLayerButton);
    
    QPushButton *copyLayerButton = new QPushButton(tr("Дублировать"), this);
    QPushButton *mergeLayerButton = new QPushButton(tr("Объединить"), this);
    panelLayout->addWidget(copyLayerButton);
    panelLayout->addWidget(mergeLayerButton);
    
    connect(copyLayerButton, &QPushButton::clicked, this, [=]() {
        if (layerItems.size() == 0) return;
        QListWidgetItem *item = layerList->item(layerList->currentRow());
        //QGraphicsItem *layer = this->parent()->parent()->property("activeLayer").value<QGraphicsItem*>();
        QGraphicsItem *layer = layerItems[layerList->currentRow()];
        if (item && layer) {
            QGraphicsPixmapItem *pixmapItem = dynamic_cast<QGraphicsPixmapItem*>(layer);
            QPixmap layerPixmap = pixmapItem->pixmap();
            QPixmap empty(layerPixmap.size());
            empty.fill(Qt::transparent);
            QGraphicsPixmapItem *layerItem = new QGraphicsPixmapItem(empty);
            addLayer(layerItem, item->text().append(" копия"));
            historySave(QVector<int>({(int)layerItems.size() - 1}));
            layerItem->setPixmap(layerPixmap);         
            historySave(QVector<int>({(int)layerItems.size() - 1}));
            updateThumbnails();
        }
    });
    
    connect(mergeLayerButton, &QPushButton::clicked, this, [=]() {
        if (layerItems.size() < 2 || layerList->currentRow() < 1) return;
        QListWidgetItem *item1 = layerList->item(layerList->currentRow());
        QListWidgetItem *item2 = layerList->item(layerList->currentRow() - 1);
        QGraphicsItem *layer1 = layerItems[layerList->currentRow()];
        QGraphicsItem *layer2 = layerItems[layerList->currentRow() - 1];
        if (item1 && item2 && layer1 && layer2) {
            QGraphicsPixmapItem *pixmapItem1 = dynamic_cast<QGraphicsPixmapItem*>(layer1);
            QGraphicsPixmapItem *pixmapItem2 = dynamic_cast<QGraphicsPixmapItem*>(layer2);
            QPixmap layerPixmap = pixmapItem2->pixmap();
            QPainter painter(&layerPixmap);
            painter.drawPixmap(layer1->boundingRect().toRect(), pixmapItem1->pixmap());
            painter.end();
            
            previousState = getCurrentState();
            QGraphicsPixmapItem *pixmapItem = dynamic_cast<QGraphicsPixmapItem*>(layerItems.last());
            QPixmap pixmap = pixmapItem->pixmap();
            QPixmap pixmapRestore(pixmap);
            pixmap.fill(Qt::transparent);
            pixmapItem->setPixmap(pixmap);
            historySave();
            if (layerList->currentRow() + 1 < layerItems.size()) {
                QGraphicsPixmapItem *pixmapItem = dynamic_cast<QGraphicsPixmapItem*>(layerItems.last());
                pixmapItem->setPixmap(pixmapRestore);                
            }
            
            QGraphicsPixmapItem *layerItem = new QGraphicsPixmapItem(layerPixmap);
            addLayer(layerItem, item2->text().append(" + ").append(item1->text()));
            
            scene->removeItem(layer1);
            layerItems.removeOne(layer1);
            scene->removeItem(layer2);
            layerItems.removeOne(layer2);
            
            delete item1;
            delete item2;   // Удаляем элемент списка
            historySave();            
        }
    });
    
    connect(addLayerButton, &QPushButton::clicked, this, [=]() {
        addLayer();
        historySave({0, (int)layerItems.size() - 1});});
    connect(removeLayerButton, &QPushButton::clicked, this, [=]() {
        int row = layerList->currentRow();
        previousState = getCurrentState({(int)layerItems.size() - 1});
        QGraphicsPixmapItem *pixmapItem = dynamic_cast<QGraphicsPixmapItem*>(layerItems.last());
        QPixmap pixmap = pixmapItem->pixmap();
        QPixmap pixmapRestore(pixmap);
        pixmap.fill(Qt::transparent);
        pixmapItem->setPixmap(pixmap);
        historySave();
        if (row + 1 < layerItems.size()) {
            QGraphicsPixmapItem *pixmapItem = dynamic_cast<QGraphicsPixmapItem*>(layerItems.last());
            pixmapItem->setPixmap(pixmapRestore);
        }
        removeLayer();
        updateThumbnails();
        historySave();});
    
    addLayerButton->click();
    addLayerButton->click();
    layerList->setCurrentRow(1);
    historyClear();
}

void LayerManager::addLayer() {
    QSize size(0, 0);
    if (this->parent()->parent())
        size = this->parent()->parent()->property("canvasSize").value<QSize>();
    else size = this->parent()->property("canvasSize").value<QSize>();
    QPixmap layerPixmap(size);
    if (layerItems.size() == 0) layerPixmap.fill(Qt::black);
    else layerPixmap.fill(Qt::transparent);
    QGraphicsPixmapItem *layerItem = new QGraphicsPixmapItem(layerPixmap);
    if (layerItems.size() == 0)
        addLayer(layerItem, "Выделение");
    else
        addLayer(layerItem, QString::number(layerList->count()).append(" слой"));
}

void LayerManager::addLayer(QGraphicsItem *layer, QString layerName) {
    QListWidgetItem *item = new QListWidgetItem(layerName, layerList);
    item->setFlags(item->flags() | Qt::ItemFlag::ItemIsEditable);
    layerList->addItem(item);
 
    layer->setZValue(layerItems.size() * 2);
    scene->addItem(layer);
    layerItems.push_back(layer);
    
    layerList->setCurrentRow(layerList->count() - 1);
    updateThumbnails();
}

void LayerManager::addLayerFromFile(const QString &filePath) {
    QPixmap filePixmap(filePath);
    QPixmap layerPixmap(parent()->parent()->property("canvasSize").value<QSize>());
    layerPixmap.fill(Qt::transparent);
    
    QGraphicsPixmapItem *layerItem = new QGraphicsPixmapItem(layerPixmap);
    QString layerName = QFileInfo(filePath).fileName();
    addLayer(layerItem, layerName);
    historySave({(int)layerItems.size() - 1});
    
    
    QPainter painter(&layerPixmap);
    painter.drawPixmap(0, 0, filePixmap.size().width(), filePixmap.size().height(), filePixmap);
    painter.end();
    layerItem->setPixmap(layerPixmap);
    
    updateThumbnails();
    historySave({(int)layerItems.size() - 1});
}

void LayerManager::removeLayer() {
    if (layerItems.size() == 0) return;
    QListWidgetItem *item = layerList->item(layerList->currentRow());
    QGraphicsItem *layer = layerItems[layerList->currentRow()];
    if (item && layer) {
        scene->removeItem(layer);
        layerItems.removeOne(layer);
        
        delete item;   // Удаляем элемент списка
    }
}

void LayerManager::removeLayer(int index) {
    if (layerItems.size() == 0) return;
    QListWidgetItem *item = layerList->item(index);
    QGraphicsItem *layer = layerItems[layerList->currentRow()];
    if (item && layer) {
        scene->removeItem(layer);
        layerItems.removeOne(layer);
        
        delete item;   // Удаляем элемент списка
    }
}

void LayerManager::updateThumbnails() {
    QGraphicsItem *mask = nullptr;
    if (layerItems.size() > 0) mask = layerItems[0];
    if (parent()->parent())
        parent()->parent()->setProperty("maskLayer", QVariant::fromValue(mask));
    else
        parent()->setProperty("maskLayer", QVariant::fromValue(mask));
    
    for (int i = 0; i <  layerItems.size(); i++) {
        int size = 48;
        
        QListWidgetItem *item = layerList->item(i);
        if (i == 0 ^ layerList->currentRow() == 0) layerItems[i]->setVisible(false);
        else layerItems[i]->setVisible(true);
        QPixmap thumbnail = QPixmap(size, size);
        thumbnail.fill(Qt::transparent);
        QPainter painter(&thumbnail);
        QPixmap original = dynamic_cast<QGraphicsPixmapItem*>(layerItems[i])->pixmap().scaled(size, size,
                                                                                              Qt::KeepAspectRatio, Qt::SmoothTransformation);
        painter.drawPixmap(size / 2 - original.width()  / 2,
                           size / 2 - original.height() / 2,
                           original.width(),
                           original.height(),
                           original);
        painter.end();
        if (item) item->setIcon(QIcon(thumbnail));
    }
}

void LayerManager::resize(int imageWidth, int imageHeight, int canvasWidth, int canvasHeight) {
    bool isNewSize = false;
    for (auto item : layerItems) {
        QGraphicsPixmapItem *pixmapItem = dynamic_cast<QGraphicsPixmapItem*>(item);
        if (pixmapItem->pixmap().size() != QSize(canvasWidth, canvasHeight)) isNewSize = true;
        if (pixmapItem) {
            QPixmap pixmap(canvasWidth, canvasHeight);
            if (pixmapItem->zValue() == 0) pixmap.fill(Qt::black);
            else pixmap.fill(Qt::transparent);
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            painter.drawPixmap(0, 0, imageWidth, imageHeight, pixmapItem->pixmap());
            painter.end();
            pixmapItem->setPixmap(pixmap);
            scene->setSceneRect(0, 0, canvasWidth, canvasHeight);
        }
    }
    if (isNewSize) historyClear();
}

void LayerManager::saveLayers() {
    QString folderPath = QFileDialog::getExistingDirectory(this, tr("Выберите папку для сохранения"));
    if (folderPath.isEmpty()) return;
    
    QString prefix = QInputDialog::getText(this, tr("Сохранение"), tr("Внимание: все существующие файлы "
                                                                      "с этим префиксом будут удалены/перезаписаны."
                                                                      "\n\nПрефикс для файлов:"));
    if (prefix.isEmpty()) return;
    
    QRegularExpression regex(QString(prefix).append(R"((.+)(\d+)\.\s(.+))"));

    // Поиск всех файлов с тем же префиксом
    QDir dir(folderPath);
    QStringList allFiles = dir.entryList(QDir::Files, QDir::Name);
    for (const QString &file : allFiles) {
        QRegularExpressionMatch fileMatch = regex.match(file);
        if (fileMatch.hasMatch()) {
            QString filePath = folderPath + "/" + file;
            QFile(filePath).remove();
        }
    }
    
    for (int i = 0; i < layerItems.size(); ++i) {
        QGraphicsPixmapItem *pixmapItem = dynamic_cast<QGraphicsPixmapItem*>(layerItems[i]);
        if (!pixmapItem) continue;
        
        QPixmap pixmap = pixmapItem->pixmap();
        QString layerName = layerList->item(i)->text();
        QString fileName = QString("%1/%2 %3. %4.png").arg(folderPath).arg(prefix).arg(i).arg(layerName);
        
        if (!pixmap.save(fileName)) {
            qDebug("Не удалось сохранить слой %s в файл %s", layerName.toStdString().c_str(), fileName.toStdString().c_str());
        }
    }
}

bool compareFileNames(const QString &file1, const QString &file2) {
    QRegularExpression regex(".*?(\\d+).*");
    QRegularExpressionMatch match1 = regex.match(file1);
    QRegularExpressionMatch match2 = regex.match(file2);
    
    if (match1.hasMatch() && match2.hasMatch()) {
        int number1 = match1.captured(1).toInt();
        int number2 = match2.captured(1).toInt();
        return number1 < number2;
    }
    return file1 < file2; // Лексикографический порядок, если нет чисел
}

void LayerManager::openLayers() {
    QString firstFile = QFileDialog::getOpenFileName(this, tr("Выберите первый файл слоя"), QString(), tr("Изображения (*.png *.jpg *.bmp)"));
    if (firstFile.isEmpty()) return;
    
    QFileInfo fileInfo(firstFile);
    QString folder = fileInfo.absolutePath();
    QString fileName = fileInfo.fileName();
    
    // Разбор имени файла: "префикс номер. имя"
    QRegularExpression regex(R"((.+)\s(\d+)\.\s(.+))");
    QRegularExpressionMatch match = regex.match(fileName);
    if (!match.hasMatch()) {
        qDebug("Файл не соответствует шаблону 'префикс номер. имя'");
        return;
    }
    
    historyClear();
    
    while (layerItems.size() > 0) removeLayer();
    
    QString prefix = match.captured(1);
    QString extension = fileInfo.suffix();
    
    // Поиск всех файлов с тем же префиксом
    QDir dir(folder);
    QStringList filters = { QString("*.%1").arg(extension) };
    QStringList allFiles = dir.entryList(filters, QDir::Files);
    std::sort(allFiles.begin(), allFiles.end(), compareFileNames);
    
    QSize size(0, 0);
    for (const QString &file : allFiles) {
        QRegularExpressionMatch fileMatch = regex.match(file);
        if (fileMatch.hasMatch() && fileMatch.captured(1) == prefix) {
            QString layerName = fileMatch.captured(3);
            layerName.truncate(layerName.length() - QFileInfo(file).suffix().length() - 1);
            QString filePath = folder + "/" + file;
            
            // Загрузка слоя
            QPixmap pixmap(filePath);
            size.setWidth(pixmap.width());
            size.setHeight(pixmap.height());
            
            if (!pixmap.isNull()) {
                QGraphicsPixmapItem *layer = new QGraphicsPixmapItem(pixmap);
                addLayer(layer, layerName);
            } else {
                qDebug("Не удалось загрузить слой из файла %s", filePath.toStdString().c_str());
            }
        }
    }
    parent()->parent()->setProperty("canvasSize", size);
    resize(size.width(), size.height(), size.width(), size.height());
    historyClear();
}

void LayerManager::invertLayer() {
    int currentIndex = layerList->currentRow();
    if (currentIndex < 0 || currentIndex >= layerItems.size()) return;
        
    QGraphicsPixmapItem *pixmapItem = dynamic_cast<QGraphicsPixmapItem*>(layerItems[currentIndex]);
    if (!pixmapItem) return;
    
    previousState = getCurrentState();    
    
    QPixmap pixmap = pixmapItem->pixmap();
    QImage image = pixmap.toImage();
    
    // Инвертируем изображение
    if (currentIndex == 0) {
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                QColor color = image.pixelColor(x, y);
                color.setAlpha(255 - color.alpha());
                image.setPixelColor(x, y, color);
            }
        }
    }
    else {
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                QColor color = image.pixelColor(x, y);
                color.setRed(255 - color.red());
                color.setGreen(255 - color.green());
                color.setBlue(255 - color.blue());
                image.setPixelColor(x, y, color);
            }
        }
    }
    
    pixmapItem->setPixmap(QPixmap::fromImage(image));
    updateThumbnails();
    historySave({currentIndex});    
}

void LayerManager::adjustBrightnessContrast(int brightness, double contrast) {
    int currentIndex = layerList->currentRow();
    if (currentIndex < 0 || currentIndex >= layerItems.size()) return;
    
    QGraphicsPixmapItem *pixmapItem = dynamic_cast<QGraphicsPixmapItem*>(layerItems[currentIndex]);
    if (!pixmapItem) return;
    
    previousState = getCurrentState();    
    
    QPixmap pixmap = pixmapItem->pixmap();
    QImage image = pixmap.toImage();
    
    // Регулируем яркость и контраст
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QColor color = image.pixelColor(x, y);
            
            // Регулировка яркости
            int r = color.red() + brightness;
            int g = color.green() + brightness;
            int b = color.blue() + brightness;
            
            // Регулировка контраста
            r = 128 + contrast * (r - 128);
            g = 128 + contrast * (g - 128);
            b = 128 + contrast * (b - 128);
            
            // Ограничение значений
            color.setRed(qBound(0, r, 255));
            color.setGreen(qBound(0, g, 255));
            color.setBlue(qBound(0, b, 255));
            
            image.setPixelColor(x, y, color);
        }
    }
    
    pixmapItem->setPixmap(QPixmap::fromImage(image));
    updateThumbnails();
    historySave({currentIndex});
}

QVector<QPair<QString, QImage>> LayerManager::getCurrentState() {
    QVector<QPair<QString, QImage>> state;
    
    for (int i = 0; i < layerItems.size(); ++i) {
        QPixmap pixmap = dynamic_cast<QGraphicsPixmapItem*>(layerItems[i])->pixmap();
        QString layerName = layerList->item(i)->text();
        state.append(qMakePair(layerName, pixmap.toImage()));
    }
    
    return state;
}

QVector<QPair<QString, QImage>> LayerManager::getCurrentState(QVector<int> changedLayers) {
    QVector<QPair<QString, QImage>> state;
    QSize size(0, 0);
    if (this->parent()->parent())
        size = this->parent()->parent()->property("canvasSize").value<QSize>();
    else size = this->parent()->property("canvasSize").value<QSize>();
    
    for (int i = 0; i < layerItems.size(); ++i) {
        QImage image(0, 0); //null pixmap для пропущенного слоя
        
        if (changedLayers.contains(i)) { // Только для изменённых слоёв сохраняем актуальный пиксмап
            image = dynamic_cast<QGraphicsPixmapItem*>(layerItems[i])->pixmap().toImage();
        }
        
        QString layerName = layerList->item(i)->text();
        state.append(qMakePair(layerName, image));
    }
    
    return state;
}

QImage QImageDiff(QImage first, QImage second) {
    uchar* p1 = first.bits();
    uchar* p2 = second.bits();
    uchar* end = p1 + first.sizeInBytes();

    // Ожидаем, что компилятор воспользуется дополнительными наборами инструкций процессора - SSE или AVX
    while (p1 != end) {
        *p1 ^= *p2;
        p1++;
        p2++;
    }

    return first;
}

QVector<QPair<QString, QByteArray>> LayerManager::calculateDiff(
    const QVector<QPair<QString, QImage>> &oldState,
    const QVector<QPair<QString, QImage>> &newState) {
    
    QVector<QPair<QString, QByteArray>> diff;
    QSize size(0, 0);
    if (this->parent()->parent())
        size = this->parent()->parent()->property("canvasSize").value<QSize>();
    else size = this->parent()->property("canvasSize").value<QSize>();
    
    for (int i = 0; i < (oldState.size() > 0 ? oldState : newState).size(); i++) {
        bool oldExist = i < oldState.size();
        bool newExist = i < newState.size();
        bool oldIsNull = oldExist ? oldState[i].second.isNull() : true;
        bool newIsNull = newExist ? newState[i].second.isNull() : true;
        bool isChanged = false;
        QImage deltaImage(size, QImage::Format_ARGB32);
        deltaImage.fill(Qt::transparent);
        
        QByteArray compressedDelta;
        QBuffer buffer(&compressedDelta);
        buffer.open(QIODevice::WriteOnly);
        
        if ((!newIsNull && !oldIsNull)) {
            isChanged = true;
            
            QImage oldImage;
            
            if (oldIsNull) {
                oldImage = QImage(size, QImage::Format_ARGB32);
                oldImage.fill(Qt::transparent);
            }
            else
                oldImage = oldState[i].second;
            
            if (!newIsNull) {
                deltaImage = newState[i].second;
            }

            deltaImage = QImageDiff(deltaImage, oldImage);
        }
        
        if (isChanged) deltaImage.save(&buffer, "PNG"); // Больше всего времени в этой функции занимает кодирование PNG
        diff.append(qMakePair((oldExist ? oldState[i] : newState[i]).first, compressedDelta));
    }
    
    return diff;
}

QVector<QPair<QString, QImage>> LayerManager::applyDiff(
    const QVector<QPair<QString, QByteArray>> &diff,
    const QVector<QPair<QString, QImage>> &currentState) {
    
    QSize size(0, 0);
    if (this->parent()->parent())
        size = this->parent()->parent()->property("canvasSize").value<QSize>();
    else size = this->parent()->property("canvasSize").value<QSize>();
    QVector<QPair<QString, QImage>> newState = currentState;
    
    while (diff.size() < newState.size()) {
        newState.pop_back();
    }
    
    for (int i = 0; i < diff.size(); ++i) {
        if (diff[i].second == nullptr) qDebug("null");
        QByteArray compressedDelta = diff[i].second;
        QImage currentImage;
        
        if (compressedDelta != nullptr && !compressedDelta.isEmpty()) {
            
            if (i < newState.size() && !newState[i].second.isNull())
                currentImage = newState[i].second;
            else {
                currentImage = QImage(size, QImage::Format_ARGB32);
                currentImage.fill(Qt::transparent);
            }
            
            QImage deltaImage;
            deltaImage.loadFromData(compressedDelta, "PNG");

            currentImage = QImageDiff(currentImage, deltaImage);
        }
        else {
            currentImage = QImage(0, 0);
            
        }
        
        if (i < newState.size()) {
            newState[i].second = currentImage;
            newState[i].first = diff[i].first;
        } else {
            newState.append(qMakePair(diff[i].first, currentImage));
        }
    }
    
    return newState;
}

void LayerManager::historyClear() {
    previousState = getCurrentState();
    undoStack.clear();
    redoStack.clear();
}

void LayerManager::historySave(QVector<int> changedLayers) {
    QVector<QPair<QString, QImage>> currentState = getCurrentState(changedLayers);
    
    undoStack.push(calculateDiff(previousState, currentState));
    redoStack.clear(); // Очистка redoStack при новой операции
    
    previousState = currentState;
}

void LayerManager::historySave() {
    QVector<QPair<QString, QImage>> currentState = getCurrentState();
    
    undoStack.push(calculateDiff(previousState, currentState));    
    redoStack.clear(); // Очистка redoStack при новой операции
    
    previousState = currentState;
}

void LayerManager::historyUndo() {
    if (undoStack.isEmpty()) return;
    
    QVector<QPair<QString, QByteArray>> lastDiff = undoStack.pop();
    QVector<QPair<QString, QImage>> currentState = previousState;
    QVector<QPair<QString, QImage>> restoredState = applyDiff(lastDiff, currentState);
    
    // Сохраняем текущую разницу в redoStack
    redoStack.push(calculateDiff(currentState, restoredState));
     
    // Обновляем холст
    updateLayers(restoredState);
    previousState = getCurrentState();
}

void LayerManager::historyRepeat() {
    if (redoStack.isEmpty()) return;
    
    QVector<QPair<QString, QByteArray>> nextDiff = redoStack.pop();
    QVector<QPair<QString, QImage>> currentState = previousState;
    QVector<QPair<QString, QImage>> restoredState = applyDiff(nextDiff, currentState);
    
    // Сохраняем текущую разницу в undoStack
    undoStack.push(calculateDiff(currentState, restoredState));
    
    // Обновляем холст
    updateLayers(restoredState);
    previousState = getCurrentState();
}

void LayerManager::updateLayers(const QVector<QPair<QString, QImage>> &state) {
    // Добавление новых слоёв, если их стало больше
    while (layerItems.size() < state.size()) {
        addLayer();
    }
    
    // Удаление лишних слоёв, если их стало меньше
    while (layerItems.size() > state.size()) {
        removeLayer(layerItems.size() - 1);
    }
    
    // Обновление слоёв и их названий
    QSize size(0, 0);
    for (int i = 0; i < state.size(); ++i) {
        const QString &layerName = state[i].first;
        const QImage &image = state[i].second;
        
        QGraphicsPixmapItem *pixmapItem = dynamic_cast<QGraphicsPixmapItem*>(layerItems[i]);
        if (pixmapItem && !image.isNull()) {
            QPixmap pixmap(image.width(), image.height());
            pixmap.fill(Qt::transparent);
            QPainter painter(&pixmap);
            painter.drawImage(0, 0, image);
            painter.end();
            pixmapItem->setPixmap(pixmap);
        }
        
        layerList->item(i)->setText(layerName);
        
    }
    
    updateThumbnails();
}
