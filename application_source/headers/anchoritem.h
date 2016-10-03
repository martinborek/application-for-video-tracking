/**
 * @file anchoritem.h
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#ifndef ANCHORITEM_H
#define ANCHORITEM_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>

#include "timelabel.h"

#include <stdint.h> // int64_t; needed for MSVC compiler

enum class ItemType : int {BEGINNING, END, CHANGE};

class AnchorItem : public QWidget
{
    Q_OBJECT
public:
    /**
     * Constructor
     * @param type Item type
     * @param timestamp Timestamp
     * @param timePosition Time position
     * @param totalTime Total video time
     * @param frameNumber Fram number
     * @param frameCount Number of frames in the video
     * @param parent Parent widget
     * @param isSet Is the item set? Works only with END ItemType
     * @param displayTime True - display time. False - display frame number
     */
    AnchorItem(ItemType type, int64_t timestamp, unsigned long timePosition, unsigned long totalTime,
                   unsigned long frameNumber, unsigned long frameCount, QWidget *parent, bool isSet=true, bool displayTime=false);
    /**
     * Destructor
     */
    ~AnchorItem();

    /**
     * Returns type of this item.
     * @return Item type
     */
    ItemType get_type();

    /**
     * Is the object set? isSet works only with END ItemType.
     * @return isSet.
     */
    bool is_set();

    /**
     * Sets highlighting. Works with eventFilter().
     * @param set True - highlight always on. False - highlight set according item hover.
     */
    void set_highlight(bool set);

    /**
     * Returns timestamp.
     * @return timestamp
     */
    int64_t get_timestamp() const;

    /**
     * Displays values as time positions.
     */
    void display_time();

    /**
     * Displays values as frame numbers.
     */
    void display_frame_num();

signals:
protected:
    /**
     * Filters Enter and Leave events for item highlighting when hovering
     * @param object Object of the event
     * @param event Event
     * @return False - propagate event further. True - do not propagate.
     */
    bool eventFilter(QObject *object, QEvent *event);

private:
    TimeLabel *timeLabel;
    QLabel *name;
    ItemType type;
    int64_t timestamp;
    unsigned long timePosition;
    unsigned long frameNumber;
    bool isSet;

    QHBoxLayout *hLayout;
    const QString backgroundColorActive = "background-color: rgb(115, 171, 230);";
    const QString backgroundColorInactive = "background-color: rgb(255, 255, 255);";
    QString currentBackgroundColor;
};

#endif // ANCHORITEM_H
