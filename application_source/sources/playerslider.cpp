/**
 * @file playerslider.cpp
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#include "playerslider.h"

#include <QDebug>

PlayerSlider::PlayerSlider(QWidget *parent) : QSlider(parent)
{

}

/* When mouse is pressed at a position different than the controller, signal with
 * the position is emitted so that the video can jump to the frame.*/
/**
void PlayerSlider::mousePressEvent(QMouseEvent *event)
{
    // QSlider mousePressEvent needs to be called so that signals
    // sliderPressed with sliderMoved are emitted correctly.
    // It also sets flag SliderDown when the "controller" is pressed
    // and therefore no further action is needed since the pressed position
    // is the current one.
    QSlider::mousePressEvent(event);
    if (isSliderDown())
        return;

    if (event->button() == Qt::LeftButton)
    {

        int position;
        double value = 0; //0-1
        if (orientation() == Qt::Horizontal)
            value = event->x() / static_cast<double>(width()-1);
        else // ==Qt::Vertical
            value = event->y() / static_cast<double>(height()-1);

        // Position is more accurate when floor() is used at the beginning
        // and ceil() at the end.
        if (value < 0.3)
            position = floor(minimum() + (maximum()-minimum()) * value);
        else if (value > 0.7)
            position = ceil(minimum() + (maximum()-minimum()) * value);
        else
            position = round(minimum() + (maximum()-minimum()) * value);

        emit sliderMoved(position);

        event->accept();
    }

}
*/
void PlayerSlider::mouseReleaseEvent(QMouseEvent *event)
{
    qDebug() << "clicked";
    // QSlider mousePressEvent needs to be called so that signals
    // sliderPressed with sliderMoved are emitted correctly.
    // It also sets flag SliderDown when the "controller" is pressed
    // and therefore no further action is needed since the pressed position
    // is the current one.
    QSlider::mouseReleaseEvent(event);
    if (isSliderDown())
        return;

    if (event->button() == Qt::LeftButton)
    {

        long position;
        double value = 0; //0-1
        if (orientation() == Qt::Horizontal)
            value = event->x() / static_cast<double>(width()-1);
        else // ==Qt::Vertical
            value = event->y() / static_cast<double>(height()-1);

        // Position is more accurate when floor() is used at the beginning
        // and ceil() at the end.
        if (value < 0.3)
            position = floor(minimum() + (maximum()-minimum()) * value);
        else if (value > 0.7)
            position = ceil(minimum() + (maximum()-minimum()) * value);
        else
            position = round(minimum() + (maximum()-minimum()) * value);

        //unsigned long maximum = this->maximum();
        //unsigned long minimum = this->minimum();

        if (position > maximum())
            position = maximum();
        else if (position < minimum())
            position = minimum();

        qDebug() << "position" << position;
        unsigned long returnPosition = position;
        emit clicked(returnPosition);

        event->accept();
    }

}

PlayerSlider::~PlayerSlider()
{

}

