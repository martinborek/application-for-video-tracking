/**
 * @file trackedobject.cpp
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#include "trackedobject.h"
#include <QDebug>
#include "objectshape.h"

// Only currentSection can have initialized trackingAlgorithm so as memory can be correctly freed

TrackedObject::TrackedObject()
{ // CEREAL uses this constructor
    endTimestampSet = false;
    currentSection = nullptr;
    nextSection = false;
    allProcessed = false;
    qDebug() << "new trackedObject";
}

TrackedObject::TrackedObject(const Characteristics &appearance, std::string objectName, int64_t initialTimestamp,
                             Selection initialPosition, unsigned long initialTimePosition, unsigned long initialFrameNumber,
                             bool endTimestampSet, int64_t endTimestamp, unsigned long endTimePosition,
                             unsigned long endFrameNumber) :
    name(objectName),
    appearance(appearance),
    initialTimestamp(initialTimestamp),
    //initialPosition(initialPosition),
    endTimestampSet(endTimestampSet),
    endTimestamp(endTimestamp),
    endTimePosition(endTimePosition),
    endFrameNumber(endFrameNumber)
{
    add_section(initialTimestamp, initialPosition, initialTimePosition, initialFrameNumber);

    currentSection = nullptr;
    nextSection = false;
    allProcessed = false;
    //lastProcessedTimestamp = -1;
    //lastProcessedTimestamp = VIDEOTRACKING_NOTHING_PROCESSED;
}

TrackedObject::~TrackedObject()
{
    for (auto &section: trajectorySections)
    {
        if (section.second.trackingAlgorithm)
        {// if there is initialized trackingAlgorithm, free its memory
         // this should occur only with currentSection
            delete section.second.trackingAlgorithm;
            section.second.trackingAlgorithm = nullptr;
        }
    }
/**
    if (trackingAlgorithm)
    { // was it already initialized (by initialize_section)?
        delete trackingAlgorithm;
        trackingAlgorithm = nullptr;
    }

    */
}

void TrackedObject::add_section(int64_t initialTimestamp, Selection const &initialPosition, unsigned long initialTimePosition, unsigned long initialFrameNumber)
{

    TrajectorySection newSection(initialPosition, initialTimestamp, initialTimePosition, initialFrameNumber);

    trajectorySections.insert({initialTimestamp, newSection});

}

// If oldTimeSet==true, oldTimestamp is valid and updating is done, otherwise adding is done
bool TrackedObject::set_trajectory_section(int64_t newTimestamp, Selection position, unsigned long timePosition, unsigned long frameNumber)
{
    if (endTimestampSet && endTimestamp < newTimestamp)
        return false; // Section cannot begin after the end of the tracking period
    else if (trajectorySections.find(newTimestamp) != trajectorySections.end())
    {   // Section with this timestamp already exists
        // Change that section instead of creating a new one
        qDebug() << "set_trajectory_section calls change_trajectory_section";
        return change_trajectory_section(newTimestamp, newTimestamp, position, timePosition, frameNumber);
    }

    add_section(newTimestamp, position, timePosition, frameNumber);

    allProcessed = false;
    int64_t lastProcessedTimestamp;
    bool lastProcessedTimestampSet = get_last_processed_timestamp(lastProcessedTimestamp);

    if (newTimestamp < initialTimestamp)
    { // delete current and delete all trajectory => DONE
        initialTimestamp = newTimestamp;
        trajectory.clear();
    }
    else if ((newTimestamp < initialTimestamp) || // This section begins before the BEGINNING section. Therefore, set this section as beginning.
             (lastProcessedTimestampSet && newTimestamp <= lastProcessedTimestamp))
    {

        if (currentSection)
        { // Unset it
            if (currentSection->trackingAlgorithm)
            {
                delete currentSection->trackingAlgorithm;
                currentSection->trackingAlgorithm = nullptr;
            }
            currentSection = nullptr;
            nextSection = false;
        }

        if (newTimestamp < initialTimestamp)
        {
            initialTimestamp = newTimestamp;
            trajectory.clear();
        }
        else // (lastProcessedTimestampSet && newTimestamp <= lastProcessedTimestamp)
        {
            // delete all >=newTimestamp
            trajectory.erase(trajectory.find(newTimestamp), trajectory.end());
        }

    }
    else
    { // newTimestamp > LASTPROCESSEDTIMESTAMP
        // No need to unset currentSection or delete anything from trajectory.
        // The currently processed frame is before this section.
        // Only nextSectionTimestamp may need to be updated if the newly added section is the next one.

        // The newly added is already in trajectorySections, so let it find the appropriate one itself
        auto nextSectionIterator = trajectorySections.upper_bound(currentSection->initialTimestamp);
        if (nextSectionIterator != trajectorySections.end())
        {
            nextSection = true;
            nextSectionTimestamp = nextSectionIterator->first;
        }
        else
            nextSection = false;
    }

    return true;
}

bool TrackedObject::change_trajectory_section(int64_t oldTimestamp, int64_t newTimestamp, Selection position, unsigned long timePosition, unsigned long frameNumber)
{
    // The oldTimestamp needs to exist
    assert(trajectorySections.find(oldTimestamp) != trajectorySections.end());

    if (endTimestampSet && endTimestamp < newTimestamp)
        return false; // Section cannot begin after the end of the tracking period

    // currentSection will be unset, therefore trackingAlgorithm deletion is needed
    if (currentSection && currentSection->trackingAlgorithm)
    { // if currentSection is set and has initialized trackingAlgorithm, delete the trackingAlgorithm
      // as it is not needed anymore; if needed, it would get initialized again

        trajectory.erase(trajectory.find(currentSection->initialTimestamp), trajectory.end()); // Clear all the trajectory from Current section initial timestamp till the end
        delete currentSection->trackingAlgorithm;
        currentSection->trackingAlgorithm = nullptr;
    }

    currentSection = nullptr; // let track_next() to find correct currentSection and nextSection
    nextSection = false;
    allProcessed = false;

    auto oldSection = trajectorySections.find(oldTimestamp);

    if (oldSection == trajectorySections.begin())
    { // The Beginning section is being changed

        assert(initialTimestamp == oldTimestamp);

        trajectory.clear(); // All trajectory will be counted from the beginning

        // Delete all trajectory changes (sections) that occur before this section as this is the beginning.
        // This situation happens when user moves the Beginning forward while some Section was
        // defined on some position between the old Beginning and the new one.
        trajectorySections.erase(trajectorySections.begin(), trajectorySections.find(newTimestamp));

        initialTimestamp = newTimestamp;

    }
    else
    { // Trajectory change section is being change, not Beginning
        trajectory.erase(trajectory.find(newTimestamp), trajectory.end()); // Clear all the trajectory from the timestamp at which the section begins till the end

        if (newTimestamp < initialTimestamp) // This section begins before the BEGINNING section. Therefore, set this section as beginning.
            initialTimestamp = newTimestamp;

        if (oldTimestamp < newTimestamp)
        { // Section is updated to begin later

            // V tomhle pripade je treba, aby bylo z trajectory odstraneno vsechno pocinaje
            // zacatku PREDCHOZI sekce od oldTimestamp. To z toho duvodu, ze od oldTimestamp
            // je treba prepocitat novou trajektorii. Vzhledem k tomu, ze zadna sekce na snimku
            // oldTimestamp nezacina (ta soucasna se meni ne currentTimestamp), tak je treba pozici
            // objektu vypocitat pomoci trackingAlgorithm predchozi sekce. Ten vsak musi byt
            // inicializovan a proto musi probehnout od zacatku. Z toho duvodu se maze vse od
            // zacatku teto predchozi sekce.

            // If oldSection was trajectorySections.begin(), decreasing the iterator would cause
            // undefined behaviour. However, this branche secures that this cannot happen.
            trajectory.erase(trajectory.find((--oldSection)->first), trajectory.end());
        }

    }

    trajectorySections.erase(oldTimestamp);

    if (trajectorySections.find(newTimestamp) != trajectorySections.end())
    {   // Section with this timestamp already exists
        // Call change_trajectory_section to replace the old section by this new one
        qDebug() << "change_trajectory_section(): Section with given timestamp exists. Thus, the old one was deleted.";
        return change_trajectory_section(newTimestamp, newTimestamp, position, timePosition, frameNumber);
    }

    add_section(newTimestamp, position, timePosition, frameNumber);

    return true;
}

/* If false return: Beginning section cannot be deleted */
bool TrackedObject::delete_trajectory_section(int64_t timestamp)
{
    if (timestamp == initialTimestamp) // Beginning section cannot be deleted
    {
        qDebug() << "Beginning section cannot be deleted";
        return false;
    }

    if (currentSection && (currentSection->initialTimestamp >= timestamp))
    { // This trajectorySection will be erased below(==timestamp) or
      // takes place after the section being deleted(>timestamp), therefore needs to be unset
        if (currentSection->trackingAlgorithm)
        {
            delete currentSection->trackingAlgorithm;
            currentSection->trackingAlgorithm = nullptr;
        }

        currentSection = nullptr;
        nextSection = false;
    }
    else if (nextSection && nextSectionTimestamp == timestamp)
    {   // The deleted section should have been the next section.
        // Set the following section or false if no later section exists.

        auto nextSectionIterator = trajectorySections.upper_bound(timestamp);
        if (nextSectionIterator != trajectorySections.end())
        {
            nextSection = true;
            nextSectionTimestamp = nextSectionIterator->first;
        }
        else
            nextSection = false;
    }
    allProcessed = false;

    // previousSection always exists as Beginning section cannot be deleted.
    auto previousSection = --(trajectorySections.find(timestamp));
    trajectorySections.erase(timestamp);

    // Remove all entries that exist for frames at timestamps higher than the beginning
    // of the PREVIOUS section. It is needed so the trackingAlgorithm from the previous section
    // goes from its beginning to have the correct data. Thereby, the deleted trajectory will be recounted.
    trajectory.erase(trajectory.find(previousSection->first), trajectory.end());

    return true;
}

/* If set==false, the end frame will be unset and the object will be tracked till the end of the video
 * newTimestamp and timePosition are valid only if set==true
 * Returns false when given timestamp is lower than the timestamp of Beginning
 */
bool TrackedObject::change_end_frame(bool set, int64_t timestamp, unsigned long timePosition, unsigned long frameNumber)
{
    if (!set)
    { // Tracking till the end of the video

        if (allProcessed)
        {   // All frames were processed, but now the length is extended.
            // Delete trajectory from the last section's initial timestamp till the end so the
            // tracking algorithm has the right values.

            trajectory.erase(trajectory.find(trajectorySections.rbegin()->first), trajectory.end());

        }

        endTimestampSet = false;
        allProcessed = false;
    }
    else
    { // End timestamp is set
        if (timestamp < initialTimestamp)// given timestamp is lower than the timestamp of Beginning -> Not valid
            return false;

        if (allProcessed && timestamp > endTimestamp)
        {   // All frames were processed, but now the length is extended.
            // Delete trajectory from the last section's initial timestamp till the end so the
            // tracking algorithm has the right values.

            trajectory.erase(trajectory.find(trajectorySections.rbegin()->first), trajectory.end());

        }

        allProcessed = false;

        endTimestampSet = true;
        endTimestamp = timestamp;

        endTimePosition = timePosition;
        endFrameNumber = frameNumber;

        // Remove all entries that exist for frames at timestamps higher than the end timestamp
        trajectory.erase(trajectory.upper_bound(timestamp), trajectory.end());

        if (currentSection && (currentSection->initialTimestamp > timestamp) && currentSection->trackingAlgorithm)
        { // This trajectorySection will be erased below, therefore needs to be unset
            delete currentSection->trackingAlgorithm;
            currentSection->trackingAlgorithm = nullptr;
            currentSection = nullptr;
            nextSection = false;
        }

        if (nextSection && (nextSectionTimestamp > endTimestamp)) // If there is a next section that is later than the end timestamp
            nextSection = false;

        // Remove all sections (trajectory changes) that have timestamp higher than the end timestamp
        trajectorySections.erase(trajectorySections.upper_bound(timestamp), trajectorySections.end());


        if (trajectory.find(timestamp) != trajectory.end())
        { // "trajectory" contains positions throughout all the object live; thus, everything is processed
            set_all_processed(true);
        }


    }

    return true;
}

void TrackedObject::change_appearance(Characteristics const &newAppearance)
{
    appearance = newAppearance;
}

Characteristics TrackedObject::get_appearance() const
{
    return appearance;
}

//Selection TrackedObject::track_next(cv::Mat const &frame, int64_t timestamp)

// All changes to trajectorySections make currentSection==nullptr and nextSection=false, so
// track_next() needs to find appropriate values
Selection TrackedObject::track_next(VideoFrame const *frame)
{
    if (!currentSection || (nextSection && frame->get_timestamp() >= nextSectionTimestamp))
    { // Enters new section
      // Close old section and initialize a new one.

        int64_t currentSectionTimestamp;
        if (!currentSection)
        { // First section or change to trajectorySections was made
            if (trajectory.begin() == trajectory.end())
            { // Nothing is processed, set Beginning section
                auto trajectoryIterator = trajectorySections.begin();
                currentSection = &(trajectoryIterator->second);

                currentSectionTimestamp = trajectoryIterator->first; // Is later used to count the nextSectionTimestamp
            }
            else
            {
                int64_t lastProcessed = trajectory.rbegin()->first;

                // Find section that begins after the last processed frame.
                // That should be the next frame. If not - entries would be missing
                // and that would cause an error.
                auto trajectoryIterator = trajectorySections.upper_bound(lastProcessed);
                currentSection = &(trajectoryIterator->second);
                currentSectionTimestamp = trajectoryIterator->first;

            }
        }
        else
        { // currentSection was set
            // Entering next section
            if (currentSection->trackingAlgorithm)
            { // delete the trackingAlgorithm as it is not needed anymore;
              // if needed, it would get initialized again
                delete currentSection->trackingAlgorithm;
                currentSection->trackingAlgorithm = nullptr;
            }
            currentSection = &(trajectorySections[nextSectionTimestamp]);
            currentSectionTimestamp = nextSectionTimestamp;
        }

        auto nextSectionIterator = trajectorySections.upper_bound(currentSectionTimestamp);
        if (nextSectionIterator != trajectorySections.end())
        {
            nextSection = true;
            nextSectionTimestamp = nextSectionIterator->first;
        }
        else
            nextSection = false;

        // Now initialize the new section's tracking algorithm
        Selection centerizedPosition;


        currentSection->trackingAlgorithm = new TrackingAlgorithm(*(frame->get_mat_frame()),
                                                            currentSection->initialPosition, centerizedPosition);

        trajectory[frame->get_timestamp()] = TrajectoryEntry(centerizedPosition, frame->get_time_position(), frame->get_frame_number());

        return centerizedPosition;
    }

    Selection result = currentSection->trackingAlgorithm->track_next_frame(*(frame->get_mat_frame()));

    trajectory[frame->get_timestamp()] = TrajectoryEntry(result, frame->get_time_position(), frame->get_frame_number());


    int64_t lastProcessedTimestamp;
    if (endTimestampSet && get_last_processed_timestamp(lastProcessedTimestamp) && lastProcessedTimestamp == endTimestamp)
    {
        set_all_processed(true);
    }

    return result;
}

bool TrackedObject::get_position(int64_t timestamp, Selection &trackedPosition) const
{
    try
    {
        trackedPosition = trajectory.at(timestamp).position; // Throws an exception, when trajectory with given timestamp does not exist
    } catch (std::out_of_range) {
        return false;
    }

    return true;
}

bool TrackedObject::draw_mark(cv::Mat &frame, cv::Mat const &originalFrame, int64_t timestamp) const
{
    //qDebug()<< "Draw mark";

    Selection position;
    try
    {
        position = trajectory.at(timestamp).position; // Throws an exception, when trajectory with given timestamp does not exist
    } catch (std::out_of_range) {
        return false;
    }

    if (appearance.defocus)
    {
        assert(appearance.defocusSize > 0);

        //cv::Mat3b roiMat = imgMat(cv::Rect(hSt,vSt,hEn,vEn));
        unsigned long x = position.x - position.width/2;
        unsigned long y = position.y - position.height/2;
        unsigned long xMax = position.width + x;
        unsigned long yMax = position.height + y;

        // This secures valid values
        unsigned long cols = frame.cols;
        unsigned long rows = frame.rows;
        if (xMax > cols)
            xMax = cols;
        if (yMax > rows)
            yMax = rows;

        unsigned long i, j;
        unsigned long currentSquareWidth, currentSquareHeight;
        cv::Rect roiRect;
        cv::Mat roi;
        cv::Scalar mean;
        for (i=x; i < xMax; i+=appearance.defocusSize)
        {
            for (j=y; j < yMax; j+=appearance.defocusSize)
            {
                currentSquareWidth = appearance.defocusSize;
                currentSquareHeight = appearance.defocusSize;

                if (i+currentSquareWidth > xMax) // Do not cross the object's border
                    currentSquareWidth = xMax - i;

                if (j+currentSquareHeight > yMax) // Do not cross the object's border
                    currentSquareHeight = yMax - j;

                // Width and height must not be 0 for roi to be created
                if (currentSquareWidth > 0 && currentSquareHeight > 0)
                {
                    roiRect = cv::Rect(i, j, currentSquareWidth, currentSquareHeight);
                    roi = originalFrame(roiRect); // originalFrame is used so as it does not involve other tracked objects
                    mean = cv::mean(roi);
                    cv::rectangle(frame, roiRect, mean, CV_FILLED);
                }
            }
        }

    }
    else // Fill and/or border; not defocus
    {
        cv::RotatedRect rectangle = cv::RotatedRect(cv::Point2f(position.x, position.y), cv::Size2f(position.width, position.height), position.angle);

        // Important note: OpenCV uses BGR, not RGB
        cv::Scalar borderColor(appearance.borderColor.b, appearance.borderColor.g, appearance.borderColor.r);
        cv::Scalar color(appearance.color.b, appearance.color.g, appearance.color.r);

        if (appearance.shape == ObjectShape::RECTANGLE)
        {
            cv::Point2f vertices2f[4];
            cv::Point vertices[4];
            rectangle.points(vertices2f);
            for (int i = 0; i < 4; i++)
                vertices[i] = vertices2f[i];

            if (appearance.drawInside)
                cv::fillConvexPoly(frame, vertices, 4, color);


            if (appearance.drawBorder && appearance.borderThickness > 0)
            {
                for (int i = 0; i < 4; i++)
                    cv::line(frame, vertices2f[i], vertices2f[(i+1)%4], borderColor, appearance.borderThickness, CV_AA);
            }
        }
        else if (appearance.shape == ObjectShape::ELLIPSE)
        {
            if (appearance.drawInside)
                cv::ellipse(frame,rectangle, color, -1);

            if (appearance.drawBorder && appearance.borderThickness > 0)
                cv::ellipse(frame,rectangle, borderColor, appearance.borderThickness);

        }
    }

    return true;
}

std::map<int64_t, TrajectorySection> const &TrackedObject::get_trajectory_sections() const
{
    return trajectorySections;
}

std::map<int64_t, TrajectoryEntry> const &TrackedObject::get_trajectory() const
{
    return trajectory;
}

int64_t TrackedObject::get_initial_timestamp() const
{
    return initialTimestamp;
}

int64_t TrackedObject::get_end_timestamp() const
{
    return endTimestamp;
}

unsigned long TrackedObject::get_end_time_position() const
{
    return endTimePosition;
}

unsigned long TrackedObject::get_end_frame_number() const
{
    return endFrameNumber;
}

//int64_t TrackedObject::get_last_processed_timestamp() const

/* If false is returned, timestamp is not valid */
bool TrackedObject::get_last_processed_timestamp(int64_t &timestamp) const
{
    if (trajectory.begin() == trajectory.end())
    {
        qDebug() << "Trajectory is empty";
        return false;
    }

    timestamp = trajectory.rbegin()->first;
    return true;
    //return lastProcessedTimestamp;
}

void TrackedObject::set_all_processed(bool processed)
{
    allProcessed = processed;
    //qDebug() << "all processed set";

    if (processed == true)
    {
        if (currentSection && currentSection->trackingAlgorithm)
        { // if currentSection is set and has initialized trackingAlgorithm, delete the trackingAlgorithm
          // as it is not needed anymore; if needed, it would get initialized again
            delete currentSection->trackingAlgorithm;
            currentSection->trackingAlgorithm = nullptr;
        }
        currentSection = nullptr;
        nextSection = false;
    }
}

bool TrackedObject::is_all_processed() const
{
    return allProcessed;
}

bool TrackedObject::is_end_timestamp_set() const
{
    return endTimestampSet;
}

std::string TrackedObject::get_name() const
{
    return name;
}

bool TrackedObject::set_name(std::string newName)
{
    name = newName;

    return true;
}

void TrackedObject::erase_trajectory_to_comply()
{
    if (allProcessed) // Everything is already processed, sections are not needed anymore
        return;

    int64_t lastProcessedTimestamp;
    if (!get_last_processed_timestamp(lastProcessedTimestamp))
        return; // Nothing processed -> nothing to be erased

    auto section = --(trajectorySections.upper_bound(lastProcessedTimestamp));

    trajectory.erase(trajectory.find(section->second.initialTimestamp), trajectory.end()); // Clear all the trajectory from Current section initial timestamp till the end
}
