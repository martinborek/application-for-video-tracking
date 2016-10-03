/**
 * @file videowidget.cpp
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#include "videowidget.h"

VideoWidget::VideoWidget(QWidget *parent) : QWidget(parent)
{

}

VideoWidget::~VideoWidget()
{

}

void VideoWidget::resizeEvent(QResizeEvent *event)
{
    emit resized();

    QWidget::resizeEvent(event);
}

