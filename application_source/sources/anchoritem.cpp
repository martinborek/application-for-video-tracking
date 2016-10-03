/**
 * @file anchoritem.cpp
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#include "anchoritem.h"

#include <QDebug>
#include <QMouseEvent>

AnchorItem::AnchorItem(ItemType type, int64_t timestamp, unsigned long timePosition, unsigned long totalTime,
                               unsigned long frameNumber, unsigned long frameCount,  QWidget *parent, bool isSet, bool displayTime) :
    QWidget(parent),
    type(type),
    timestamp(timestamp),
    timePosition(timePosition),
    frameNumber(frameNumber),
    isSet(isSet)
{

    currentBackgroundColor = backgroundColorInactive;
    QString nameString;
    switch (type)
    {
    case ItemType::END:
    {
        nameString = tr("End");
        break;
    }
    case ItemType::BEGINNING:
    {
        nameString = tr("Beginning");
        break;
    }
    case ItemType::CHANGE:
        nameString = tr("Trajectory change");
        break;
    default:
        qDebug() << "ERROR - AnchorItem: Wrong ItemType";
        return;
    }

    name = new QLabel(nameString, this);
    hLayout = new QHBoxLayout(this);
    hLayout->setMargin(0);
    hLayout->setSpacing(0);
    hLayout->addWidget(name);
    setLayout(hLayout);

    if (type == ItemType::END && !isSet)
    {
        QLabel *time = new QLabel(tr("Not set"), this);
        time->setAlignment(Qt::AlignRight);
        hLayout->addWidget(time);
    }
    else
    {
        timeLabel = new TimeLabel(this);
        timeLabel->set_total_time(totalTime);
        timeLabel->set_frame_count(frameCount);
        if (displayTime)
            timeLabel->display_time(timePosition, false);
        else
            timeLabel->display_frame_num(frameNumber, false);
        timeLabel->setAlignment(Qt::AlignRight);
        hLayout->addWidget(timeLabel);
    }


    installEventFilter(this);

}

bool AnchorItem::eventFilter(QObject *object, QEvent *event)
{
    if (isEnabled() && object == this)
    {
        if (event->type() == QEvent::Enter)
        {
            setStyleSheet(backgroundColorActive);
        }
        else if(event->type() == QEvent::Leave)
        {
            setStyleSheet(currentBackgroundColor);
        }
        return false; // propagates the event even when processed
    }

    return false; // propagates the event further since was not processed
}

void AnchorItem::set_highlight(bool set)
{
    if (set)
        currentBackgroundColor = backgroundColorActive;
    else
        currentBackgroundColor = backgroundColorInactive;

    setStyleSheet(currentBackgroundColor);
}

AnchorItem::~AnchorItem()
{
    // All objects get deleted automatically as they are set to be children
}

ItemType AnchorItem::get_type()
{
    return type;
}

bool AnchorItem::is_set()
{
    return isSet;
}

/**
void AnchorItem::set_highlight(bool set)
{
    if (set)
        setStyleSheet(backgroundColorActive);
    else
        setStyleSheet(backgroundColorInactive);


    QFont nameFont = name->font();
    nameFont.setUnderline(set);
    name->setFont(nameFont);
}
*/

int64_t AnchorItem::get_timestamp() const
{
    return timestamp;
}

void AnchorItem::display_time()
{
    if (type == ItemType::END && !isSet)
    {   // End is not set so the time cannot be displayed
        return;
    }

    timeLabel->display_time(timePosition, false);
}

void AnchorItem::display_frame_num()
{
    if (type == ItemType::END && !isSet)
    {   // End is not set so the frame number cannot be displayed
        return;
    }

    timeLabel->display_frame_num(frameNumber, false);
}


/**
void AnchorItem::mouseMoveEvent(QMouseEvent *event)
{
    qDebug() << "default hover";
    if (this->rect().contains(event->pos())) {
        qDebug() << "is hover";
        name->setText("hover");
    // Mouse over Widget
    }
    else {
        name->setText("not hover");
        qDebug() << "not hover";
    // Mouse out of Widget
    }
}
*/
