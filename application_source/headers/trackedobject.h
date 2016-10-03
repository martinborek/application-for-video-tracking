/**
 * @file trackedobject.h
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#ifndef TRACKEDOBJECT_H
#define TRACKEDOBJECT_H

#include "trackingalgorithm.h"
#include "videoframe.h"

#include "selection.h"

#include <QDebug>

#include <cereal/types/string.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/tuple.hpp>

#include <cv.h>
#include <cvaux.h>
#include <cxcore.h>
#include <highgui.h>

/**
#define VIDEOTRACKING_END_OF_VIDEO -1
#define VIDEOTRACKING_ALL_PROCESSED -1
#define VIDEOTRACKING_NOTHING_PROCESSED -2
#define VIDEOTRACKING_NO_PREVIOUS_TIMESTAMP -3 // In case other function than VideoTracker::get_next_frame() is used
*/


struct TrajectorySection
{
    /**
     * CEREAL serialization
     */
    template<class Archive>
    void serialize(Archive &archive)
    {
        archive(CEREAL_NVP(initialPosition), CEREAL_NVP(initialTimestamp),
                CEREAL_NVP(initialTimePosition), CEREAL_NVP(initialFrameNumber));
    }

    /**
     * Constructor
     */
    TrajectorySection()
    {
        trackingAlgorithm = nullptr;
    }

    /**
     * Constructor
     * @param initialPosition Initial object position
     * @param initialTimestamp
     * @param initialTimePosition
     * @param initialFrameNumber
     */
    TrajectorySection(Selection initialPosition, int64_t initialTimestamp,
                      unsigned long initialTimePosition, unsigned long initialFrameNumber) :
        initialPosition(initialPosition),
        initialTimestamp(initialTimestamp),
        initialTimePosition(initialTimePosition),
        initialFrameNumber(initialFrameNumber)
    {
        trackingAlgorithm = nullptr;
    }

    Selection initialPosition;
    int64_t initialTimestamp;
    unsigned long initialTimePosition;
    unsigned long initialFrameNumber;
    TrackingAlgorithm *trackingAlgorithm;
//    bool algorithmInitialized;
};

struct TrajectoryEntry
{
    /**
     * CEREAL serialization
     */
    template<class Archive>
    void serialize(Archive &archive)
    {
        archive(CEREAL_NVP(position), CEREAL_NVP(timePosition), CEREAL_NVP(frameNumber));
    }

    /**
     * Constructor
     */
    TrajectoryEntry() { }

    /**
     * Constructor
     * @param position Object position
     * @param timePosition
     * @param frameNumber
     */
    TrajectoryEntry(Selection position, unsigned long timePosition, unsigned long frameNumber) :
        position(position),
        timePosition(timePosition),
        frameNumber(frameNumber)
    { }

    Selection position;
    unsigned long timePosition;
    unsigned long frameNumber;

};

class TrackedObject
{

public:
    /**
     * CEREAL serialization
     */
    template<class Archive>
    void serialize(Archive &archive)
    {
        archive(CEREAL_NVP(name), CEREAL_NVP(appearance), CEREAL_NVP(initialTimestamp),
                CEREAL_NVP(endTimestampSet), CEREAL_NVP(endTimestamp), CEREAL_NVP(endTimePosition),
                CEREAL_NVP(endFrameNumber), CEREAL_NVP(trajectorySections), CEREAL_NVP(allProcessed),
                CEREAL_NVP(trajectory));
    }

    /**
     * Constructor
     */
    TrackedObject();

    /**
     * Constructor
     * @param appearance Object appearance
     * @param objectName Object name
     * @param initialTimestamp Initial timestamp
     * @param initialPosition Initial object position
     * @param initialTimePosition Initial time position
     * @param initialFrameNumber Initial frame number
     * @param endTimestampSet Is end timestamp set?
     * @param endTimestamp End timestamp
     * @param endTimePosition End time position
     * @param endFrameNumber End frame number
     */
    TrackedObject(Characteristics const &appearance, std::string objectName, int64_t initialTimestamp,
                  Selection initialPosition, unsigned long initialTimePosition, unsigned long initialFrameNumber,
                  bool endTimestampSet, int64_t endTimestamp, unsigned long endTimePosition,
                  unsigned long endFrameNumber);

    /**
     * Destructor
     */
    ~TrackedObject();

    /**
     * Adds a trajectory section of the object
     * @param initialTimestamp Initial timestamp of the section
     * @param initialPosition Object position at the initial timestamp
     * @param initialTimePosition Initial time position of the section
     * @param initialFrameNumber Initial frame number of the section
     */
    void add_section(int64_t initialTimestamp, Selection const &initialPosition, unsigned long initialTimePosition, unsigned long initialFrameNumber);

    /**
     * Sets a trajectory section of the object.
     * @param newTimestamp New timestamp of the section
     * @param position Object position at the first frame of the section
     * @param timePosition Time position of the section
     * @param frameNumber Frame number of the section
     * @return True if successful
     */
    bool set_trajectory_section(int64_t newTimestamp, Selection position, unsigned long timePosition, unsigned long frameNumber);

    /**
     * Changes a trajectory section of the object.
     * @param oldTimestamp Old timestamp of the section
     * @param newTimestamp New timestamp of the section
     * @param position Object position at the first frame of the section
     * @param timePosition Time position of the section
     * @param frameNumber Frame number of the section
     * @return True if successful
     */
    bool change_trajectory_section(int64_t oldTimestamp, int64_t newTimestamp, Selection position, unsigned long timePosition, unsigned long frameNumber);

    /**
     * Changes the end frame of the object.
     * @param set Set / unset last frame
     * @param timestamp Timestamp of the last frame
     * @param timePosition Time position of the last frame
     * @param frameNumber Frame number of the last frame
     * @return True if successful
     */
    bool change_end_frame(bool set, int64_t timestamp=0, unsigned long timePosition=0, unsigned long frameNumber=0);

    /**
     * Changes appearance of the object.
     * @param newAppearance New appearance of the object
     */
    void change_appearance(Characteristics const &newAppearance);

    /**
     * Returns appearance of the object.
     * @return Object appearance
     */
    Characteristics get_appearance() const;

    /**
     * Returns position of the object at frame with given timestamp.
     * @param timestamp Frame timestamp
     * @param trackedPosition Returned object position
     * @return False if trajectory with given timestamp does not exist, otherwise true.
     */
    bool get_position(int64_t timestamp, Selection &trackedPosition) const;

    /**
     * Computes position of the object in the next frame.
     * @param frame Next frame
     * @return Tracked object position
     */
    Selection track_next(VideoFrame const *frame);

    /**
     * Draws mark of the object in a frame.
     * @param frame Frame for drawing
     * @param originalFrame Original frame
     * @param timestamp Timestamp of the frame
     * @return True if successful
     */
    bool draw_mark(cv::Mat &frame, cv::Mat const &originalFrame, int64_t timestamp) const;

    /**
     * Returns trajectory sections of the object.
     * @return Trajectory section of the object
     */
    std::map<int64_t, TrajectorySection> const &get_trajectory_sections() const;

    /**
     * Returns computed trajectory of the object.
     * @return Trajectory of the object
     */
    std::map<int64_t, TrajectoryEntry> const &get_trajectory() const;

    /**
     * Returns initial timestamp.
     * @return initial timestamp
     */
    int64_t get_initial_timestamp() const;

    /**
     * Returns end timestamp.
     * @return End timestamp
     */
    int64_t get_end_timestamp() const;

    /**
     * Returns end time position.
     * @return End time position
     */
    unsigned long get_end_time_position() const;

    /**
     * Returns end frame number.
     * @return End frame number
     */
    unsigned long get_end_frame_number() const;

    /**
     * Returns last processed timestamp.
     * @param timestamp Returned last processed timestamp
     * @return Is returned timestamp valid?
     */
    bool get_last_processed_timestamp(int64_t &timestamp) const;

    /**
     * Sets value of the flag saying whether all trajectory is processed.
     * @param processed Set / unset
     */
    void set_all_processed(bool processed);

    /**
     * Returns whether all trajectory is processed.
     * @return Is all trajectory processed?
     */
    bool is_all_processed() const;

    /**
     * Returns whether the end timestamp is set.
     * @return Is end timestamp set?
     */
    bool is_end_timestamp_set() const;

    /**
     * Deletes a trajectory section.
     * @param timestamp Trajectory section to be deleted begins at this timestamp
     * @return True if successful
     */
    bool delete_trajectory_section(int64_t timestamp);

    /**
     * Returns name of the object.
     * @return  Object name
     */
    std::string get_name() const;

    /**
     * Sets name of the object.
     * @param newName New object name
     * @return True if successful
     */
    bool set_name(std::string newName);

    /**
     * Erases a part of the computed trajectory. This is necessary after deserialization (with CEREAL)
     * as track_next() initializes correct sections only when the trajectory's last frame is the last
     * frame of the previous section.
     */
    void erase_trajectory_to_comply();

private:
    std::string name;
    Characteristics appearance;

    int64_t initialTimestamp;
    bool endTimestampSet; // if FALSE -> track till the end of the video
    int64_t endTimestamp;
    unsigned long endTimePosition;
    unsigned long endFrameNumber;

    std::map<int64_t, TrajectorySection> trajectorySections;
    std::map<int64_t, TrajectoryEntry> trajectory;

    TrajectorySection *currentSection;
    bool nextSection;
    int64_t nextSectionTimestamp;
    bool allProcessed;
};

#endif // TRACKEDOBJECT_H
