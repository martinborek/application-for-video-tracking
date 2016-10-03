/**
 * @file timelabel.cpp
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#include "timelabel.h"
#include <QDebug>

void TimeConversion::convert_time(Time &converted)
{
    converted.hr = converted.raw / ms2hr;
    unsigned long rest = converted.raw % ms2hr;
    converted.min = rest / ms2min;
    rest %= ms2min;
    //converted.sec = (rest % ms2min) / ms2sec;
    converted.sec = rest / ms2sec;
    converted.ms = rest % ms2sec;
}

void TimeConversion::Time2QString(Time const &time, QString &timeString, bool showHours)
{

    timeString = time.hr ? "%1:%2:%3.%4" : "%1:%2.%3";

    if (showHours)
        timeString = timeString.arg(time.hr);

    timeString = timeString.arg(time.min, (showHours ? 2 : 1), 10, QChar('0'));

    timeString = timeString.arg(time.sec, 2, 10, QChar('0')); // Second argument ~ number of digits
    timeString = timeString.arg(time.ms/100, 1, 10, QChar('0')); // Second argument ~ number of digits
}

TimeLabel::TimeLabel(QWidget *parent) : QLabel(parent)
{
    showHours = true;
    totalTime.raw = 0;
    currentTime.raw = 0;
}

TimeLabel::~TimeLabel()
{

}


void TimeLabel::set_total_time(const unsigned long &time)
{
    totalTime.raw = time;
    convert_time(totalTime);

    if (totalTime.hr)
        showHours = true;
    else
        showHours = false;
    Time2QString(totalTime, totalTimeString, showHours);
}

void TimeLabel::display_time(const unsigned long &time, bool includeTotal)
{
    currentTime.raw = time;
    convert_time(currentTime);


    if (currentTime.raw > totalTime.raw && currentTime.sec > totalTime.sec)
    {
        qDebug() << "Error: Current time is bigger than total time";
        currentTime = totalTime;
    }


    Time2QString(currentTime, currentTimeString, showHours);
 //   qDebug() << "Time current:" << currentTimeString;
 //   qDebug() << "Time total:" << totalTimeString;

    if (includeTotal)
        setText(QString("%1 / %2").arg(currentTimeString).arg(totalTimeString));
    else
        setText(currentTimeString);
}

void TimeLabel::set_frame_count(unsigned long frameCount)
{
    totalFrameCount = frameCount;
}

void TimeLabel::display_frame_num(unsigned long frameNumber, bool includeTotal)
{
    if (includeTotal)
        setText(QString("%1 / %2").arg(QString::number(frameNumber)).arg(QString::number(totalFrameCount)));
    else
        setText(QString::number(frameNumber));
}
