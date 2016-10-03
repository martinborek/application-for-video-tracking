/**
 * @file playerslider.h
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#ifndef PLAYERSLIDER_H
#define PLAYERSLIDER_H

#include <QSlider>
#include <QMouseEvent>

class PlayerSlider : public QSlider
{
    Q_OBJECT
public:
    /**
     * Constructor
     * @param parent Parent widget
     */
    explicit PlayerSlider(QWidget *parent = 0);

    /**
     * Destructor
     */
    ~PlayerSlider();

protected:
    /**
     * Captures mouse release events to detect a mouse click that is processed
     * and clicked signal is emitted.
     * @param event Mouse event
     */
    void mouseReleaseEvent(QMouseEvent * event);

signals:
   /**
    * When a click is detected, this signal send a position of the click.
    * @param Position of the slider where the click was made
    */
   void clicked(unsigned long);

};

#endif // PLAYERSLIDER_H
