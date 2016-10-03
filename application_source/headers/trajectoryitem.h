/**
 * @file trajectoryitem.h
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#ifndef TRAJECTORYITEM_H
#define TRAJECTORYITEM_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>

#include "timelabel.h"
#include "selection.h"

#include <stdint.h> // int64_t; needed for MSVC compiler


class TrajectoryItem : public QWidget
{
    Q_OBJECT
public:
    /**
     * Constructor
     * @param timestamp Timestamp
     * @param timePosition Time position
     * @param totalTime Total video time
     * @param frameNumber Fram number
     * @param frameCount Number of frames in the video
     * @param parent Parent widget
     * @param displayTime True - display time. False - display frame number
     */
    TrajectoryItem(int64_t timestamp, unsigned long timePosition, unsigned long totalTime, unsigned long frameNumber, unsigned long frameCount,
                   Selection position, QWidget *parent, bool displayTime=false);

    /**
     * Destructor
     */
    ~TrajectoryItem();

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
    QLabel *type;
    int64_t timestamp;
    unsigned long timePosition;
    unsigned long frameNumber;
    bool isSet;
    QVBoxLayout *vLayout;
    const QString backgroundColorActive = "background-color: rgb(115, 171, 230);";
    const QString backgroundColorInactive = "background-color: rgb(255, 255, 255);";
};

#endif // TRAJECTORYITEM_H
