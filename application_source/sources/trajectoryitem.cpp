/**
 * @file trajectoryitem.cpp
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#include "trajectoryitem.h"

#include <QDebug>
#include <QMouseEvent>

TrajectoryItem::TrajectoryItem(int64_t timestamp, unsigned long timePosition, unsigned long totalTime,
                               unsigned long frameNumber, unsigned long frameCount, Selection position,
                               QWidget *parent, bool displayTime) :
    QWidget(parent),
    timestamp(timestamp),
    timePosition(timePosition),
    frameNumber(frameNumber)
{

    QWidget *title = new QWidget(this);
    title->setContentsMargins(0,3,0,0);
    type = new QLabel(title);
    type->setStyleSheet("QLabel { color: rgb(144, 144, 144)}");

    QHBoxLayout *hLayout = new QHBoxLayout(title);
    hLayout->setMargin(0);
    hLayout->setSpacing(0);

    timeLabel = new TimeLabel(title);
    timeLabel->set_total_time(totalTime);
    timeLabel->set_frame_count(frameCount);
    timeLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    if (displayTime)
    {
        timeLabel->display_time(timePosition, false);
        type->setText(tr("Time position: "));
    }
    else
    {
        timeLabel->display_frame_num(frameNumber, false);
        type->setText(tr("Frame number: "));
    }

    hLayout->addWidget(type);
    hLayout->addWidget(timeLabel);
    title->setLayout(hLayout);

    QString valuesString = "<FONT COLOR='#909090'>x:</FONT>";
    valuesString.append(QString::number(position.x));
    valuesString.append("<FONT COLOR='#909090'> y:</FONT>");
    valuesString.append(QString::number(position.y));
    valuesString.append("<FONT COLOR='#909090'> "+ tr("w") + ":</FONT>");
    valuesString.append(QString::number(position.width));
    valuesString.append("<FONT COLOR='#909090'> " + tr("h") + ":</FONT>");
    valuesString.append(QString::number(position.height));

    QLabel *values = new QLabel(valuesString, this);
    values->setContentsMargins(0,0,0,5);
    vLayout = new QVBoxLayout(this);
    vLayout->setMargin(0);
    vLayout->setSpacing(0);

    vLayout->addWidget(title);
    vLayout->addWidget(values);

    setLayout(vLayout);

    installEventFilter(this);
}

bool TrajectoryItem::eventFilter(QObject *object, QEvent *event)
{
    if (isEnabled() && object == this)
    {
        if (event->type() == QEvent::Enter)
        {
            setStyleSheet(backgroundColorActive);
        }
        else if(event->type() == QEvent::Leave)
        {
            setStyleSheet(backgroundColorInactive);
        }
        return false; // propagates the event even when processed
    }

    return false; // propagates the event further since was not processed
}

TrajectoryItem::~TrajectoryItem()
{
    // All objects get deleted automatically as they are set to be children
}

int64_t TrajectoryItem::get_timestamp() const
{
    return timestamp;
}

void TrajectoryItem::display_time()
{
    timeLabel->display_time(timePosition, false);
    type->setText(tr("Time position: "));
}

void TrajectoryItem::display_frame_num()
{
    timeLabel->display_frame_num(frameNumber, false);
    type->setText(tr("Frame number: "));
}
