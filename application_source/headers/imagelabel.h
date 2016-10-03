/**
 * @file imagelabel.h
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#ifndef IMAGELABEL_H
#define IMAGELABEL_H

#include "selection.h"

#include <QLabel>
#include <QMouseEvent>
//#include <QPoint>

class ImageLabel : public QLabel
{
    Q_OBJECT
public:
    /**
     * Constructor
     * @param parent Parent widget
     */
    explicit ImageLabel(QWidget *parent = 0);

    /**
     * Destructor
     */
    ~ImageLabel();

    /**
     * Sets new image. It ss called also when the MainWindow is resized.
     * @param image Original image
     * @param displayWidth Maximal allowed width (with respect to the current size of the window)
     * @param displayHeight Maximal allowed height
     */
    void set_image(QImage const &image, int displayWidth, int displayHeight);

    /**
     * Returns position of the currently selected area.
     * @return Selected area
     */
    Selection get_selection() const;

    /**
     * Enables or disables object (area) selection.
     * @param enabled If true, selection is enabled. If false, selection is disabled.
     */
    void set_selection_enabled(bool enabled);

    //void hide_selection();

protected:
    /**
     * Updates the ImageLabel when the MainWindows is resized.
     */
    void resizeEvent(QResizeEvent *);

    /**
     * Handles mouse press events over the ImageLabel area.
     * @param event
     */
    void mousePressEvent(QMouseEvent * event);

    /**
     * Handles mouse release events over the ImageLabel area.
     * @param event
     */
    void mouseReleaseEvent(QMouseEvent * event);

    /**
     * Handles mouse move events over the ImageLabel area.
     * @param event
     */
    void mouseMoveEvent(QMouseEvent * event);

private:
    /**
     * Shows the image without highlight any selection.
     */
    void show_without_selection();

    /**
     * Shows the image with highlighted selection.
     */
    void show_with_selection();

private:
    bool selectionEnabled;

    bool isPressed; // true -> mouse button pressed over the image
    //bool isSelected; // selection exists, therefore is displayed
    int posX; // x coordinate, where the selection starts
    int posY; // y coordinate, where the selection starts
    int imgWidth; // width of original (unscaled) image
    int imgHeight; // height of original (unscaled) image

    int scaledWidth; // width of image at the time of selecting
    int scaledHeight;
    Selection selection; // selection geometry - values with respect to scaledWidth and scaledHeight
    QImage scaledImage;

};

#endif // IMAGELABEL_H
