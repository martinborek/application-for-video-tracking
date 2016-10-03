/**
 * @file imagelabel.cpp
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#include "imagelabel.h"
#include <QtDebug>
#include <QPainter>

#include <cassert>

ImageLabel::ImageLabel(QWidget *parent) : QLabel(parent)
{
    //connect(this, SIGNAL(clicked()), this, SLOT(labelClicked()));
    selectionEnabled = false;
    isPressed = false;
    //isSelected = false;
    selection.angle = 0;
}

ImageLabel::~ImageLabel()
{

    qDebug() << "Img destroyed";
}

void ImageLabel::show_without_selection()
{
    setPixmap(QPixmap::fromImage(scaledImage));
    show();
}

void ImageLabel::show_with_selection()
{
    QImage editImage = scaledImage;
    QPainter painter(&editImage);


    assert(scaledHeight > 0);
    assert(scaledWidth > 0);

    double widthRatio = scaledImage.width() / static_cast<double>(scaledWidth);
    double heightRatio = scaledImage.height() / static_cast<double>(scaledHeight);


    painter.setBrush(QColor(0,0,0,170));
    painter.setPen(QColor(255,255,255,200));
    painter.drawRect(selection.x * widthRatio, selection.y * heightRatio
                     , selection.width * widthRatio, selection.height * heightRatio);
    painter.end(); // Not needed - destructs itself

    setPixmap(QPixmap::fromImage(editImage));
    show();

    return;
}

void ImageLabel::set_image(QImage const &image, int displayWidth, int displayHeight)
{
    imgWidth = image.width();
    imgHeight = image.height();

    scaledImage = image.scaled(displayWidth, displayHeight, Qt::KeepAspectRatio);

    resize(scaledImage.width(), scaledImage.height());
    //this->setGeometry(0, 0, scaledImage.width(), scaledImage.height());

    //if (isSelected)
    if (selectionEnabled)
        show_with_selection();
    else
        show_without_selection();
}

void ImageLabel::resizeEvent(QResizeEvent *event)
{
    isPressed = false; // User cannot be making a selection at the time of image resizing

    QLabel::resizeEvent(event);
}


void ImageLabel::mousePressEvent(QMouseEvent *event)
{
    event->accept();

    QLabel::mousePressEvent(event);

    if (!selectionEnabled) // Selection is disabled
        return;

//    qDebug()<<"Clicked";
    isPressed = true;
    posX = event->x();
    posY = event->y();
}

void ImageLabel::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();

    QLabel::mouseReleaseEvent(event);

    if (!selectionEnabled) // Selection is disabled
        return;

//    qDebug()<<"Released";
    isPressed = false;

 /**   selection.width = event->x() - selection.x;
    selection.height = event->y() - selection.y;

    emit send_position(selection);
    */
}

void ImageLabel::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();

    QLabel::mouseMoveEvent(event);

    if (!selectionEnabled) // Selection is disabled
        return;

    if (!isPressed) // No button pressed -> no need to process this event
        return;

    int curX = event->x() < this->size().width() ? event->x() : this->size().width()-1;
    int curY = event->y() < this->size().height() ? event->y() : this->size().height()-1;

    curX = curX > 0 ? curX : 0;
    curY = curY > 0 ? curY : 0;

    scaledWidth = scaledImage.width();
    scaledHeight = scaledImage.height();

    // count selection geometry:
    if (posX < curX)
    {
        selection.x = posX;
        selection.width = curX - posX;
    } else {
        selection.x = curX;
        selection.width = posX - curX;
    }

    if (posY < curY)
    {
        selection.y = posY;
        selection.height = curY - posY;
    } else {
        selection.y = curY;
        selection.height = posY - curY;
    }

    /**
    if (!isSelected)
    {
        emit selected(true);
        isSelected = true; // Selection is valid and can be displayed
    }
    */
    show_with_selection();

}

Selection ImageLabel::get_selection() const
{
 //   assert(isSelected == true);
    assert(selectionEnabled == true);

    double widthRatio = imgWidth / static_cast<double>(scaledWidth);
    double heightRatio = imgHeight / static_cast<double>(scaledHeight);

    Selection fullSizeSelection = selection;
    fullSizeSelection.width *= widthRatio;
    fullSizeSelection.x *= widthRatio;
    fullSizeSelection.height *= heightRatio;
    fullSizeSelection.y *= heightRatio;

    return fullSizeSelection;
}

void ImageLabel::set_selection_enabled(bool enabled)
{
    selectionEnabled = enabled;

    if (!enabled)
    {
        isPressed = false;
        show_without_selection();
    }
    else
    {
        scaledWidth = scaledImage.width();
        scaledHeight = scaledImage.height();
        selection.angle = 0;
        selection.width = scaledWidth/2;
        selection.height = scaledHeight/2;
        selection.x = scaledWidth/2 - selection.width/2;
        selection.y = scaledHeight/2 - selection.height/2;
        show_with_selection();
    }
}
/**
void ImageLabel::hide_selection()
{
    isSelected = false;
    show_without_selection();
}
*/
