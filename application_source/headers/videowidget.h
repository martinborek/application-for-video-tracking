/**
 * @file videowidget.cpp
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>

class VideoWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * Constructor
     * @param parent Parent widget
     */
    explicit VideoWidget(QWidget *parent = 0);

    /**
     * Destructor
     */
    ~VideoWidget();

    /**
     * When the widget is resized, the resized() signal is emitted.
     * @param event
     */
    void resizeEvent(QResizeEvent *event);
signals:
    /**
     * This signal is emitted when the widget is resized.
     */
    void resized();

public slots:
};

#endif // VIDEOWIDGET_H
