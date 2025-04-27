#include "selectcolorbutton.h"
#include <QWidget>
#include <QColorDialog>
#include <QVariant>

SelectColorButton::SelectColorButton(QWidget* parent) : QPushButton(parent) {
    this->color = Qt::black;
    updateColor();    
    connect(this, SIGNAL(clicked()), this, SLOT(changeColor()));
}

void SelectColorButton::updateColor() {
    setStyleSheet( "background-color: " + color.name() );
}

void SelectColorButton::changeColor() {
    QColor newColor = QColorDialog::getColor(color, parentWidget(), tr("Выберите цвет"));
    
    if ( newColor != color  && newColor.isValid() ) {
        setColor(newColor);
        parent()->parent()->parent()->setProperty("activeColor", newColor);
    }
}

void SelectColorButton::setColor( const QColor& color ) {
    this->color = color;
    updateColor();
}

const QColor& SelectColorButton::getColor() const {
    return color;
}