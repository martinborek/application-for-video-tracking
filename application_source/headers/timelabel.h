/**
 * @file timelabel.h
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#ifndef TIMELABEL_H
#define TIMELABEL_H

#include <QLabel>

namespace TimeConversion
{
    struct Time
    {
        unsigned long raw;
        unsigned long hr;
        unsigned long min;
        unsigned long sec;
        unsigned long ms;
    };
    static const unsigned long ms2hr = 3600000; // 1000*60*60 ms
    static const unsigned long ms2min = 60000; // 1000*60 ms
    static const unsigned long ms2sec = 1000; // 1000 ms

    void convert_time(Time &converted);
    void Time2QString(Time const &time, QString &timeString, bool showHours);

}

using namespace TimeConversion;

class TimeLabel : public QLabel
{
    Q_OBJECT
public:
    /**
     * Constructor
     * @param parent Parent widget
     */
    explicit TimeLabel(QWidget *parent = 0);

    /**
     * Destructor
     */
    ~TimeLabel();

    /**
     * Sets the total video time.
     * @param time Time in milliseconds
     */
    void set_total_time(const unsigned long &time);

    /**
     * Displays given time position with the total time.
     * @param time Displayed time
     * @param includeTotal If true, total length is also displayed.
     */
    void display_time(const unsigned long &time, bool includeTotal=true);

    /**
     * Sets the total number of frames.
     * @param frameCount Number of frames
     */
    void set_frame_count(unsigned long frameCount);

    /**
     * Displays given frame number with the total number of frames.
     * @param frameNumber Displayed frame number
     * @param includeTotal If true, total number of frames is also displayed.
     */
    void display_frame_num(unsigned long frameNumber, bool includeTotal=true);
private:

private:
    Time totalTime;
    Time currentTime;
    QString totalTimeString;
    QString currentTimeString;
    bool showHours;

    unsigned long totalFrameCount;

};

#endif // TIMELABEL_H
