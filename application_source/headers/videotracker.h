/**
 * @file videotracker.h
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#ifndef VIDEOTRACKER_H
#define VIDEOTRACKER_H

#include <cv.h>
#include <cvaux.h>
#include <cxcore.h>
#include <highgui.h>

#include <QObject>
#include <QProgressDialog>
#include <QApplication>

#include <memory>

#include "ffmpegplayer.h"
#include "trackingalgorithm.h"
#include "trackedobject.h"
#include "videoframe.h"
#include "selection.h"

#include <cereal/types/vector.hpp>
#include <cereal/types/memory.hpp> // for shared_ptr

struct OutputException : public std::exception{};
struct UserCanceledException : public std::exception{};

class VideoTracker
{

public:
    // CEREAL serialization
    template<class Archive>
    void serialize(Archive &archive)
    {
        archive(CEREAL_NVP(trackedObjects));
    }

    /**
     * Constructor
     */
    VideoTracker();

    /**
     * Constructor
     * @param videoAddr Path to a video file
     * @param progressDialog QT progress dialog for displaying information about video opening
     */
    explicit VideoTracker(std::string const &videoAddr, QProgressDialog const *progressDialog);

    /**
     * Destructor
     */
    ~VideoTracker();

    /**
     * Loads a new video file.
     * @param videoAddr Path to a video file
     * @param progressDialog QT progress dialog for displaying information about video opening
     */
    void load_video(std::string const &videoAddr, QProgressDialog const *progressDialog);

    /**
     * Returns frames per second value of the video.
     * @return Frames per second
     */
    double get_fps() const;

    /**
     * Returns information about number of frames.
     * @return Number of frames
     */
    unsigned long get_frame_count() const;

    /**
     * Returns information about video length.
     * @return Video length in milliseconds
     */
    unsigned long get_total_time() const;

    /**
     * Returns timestamp of the current frame.
     * @return Timestamp
     */
    int64_t get_frame_timestamp() const;

    /**
     * Returns time position of the current frame.
     * @return Time position in milliseconds
     */
    unsigned long get_time_position() const;

    /**
     * Returns frame number of the current frame.
     * @return Frame number
     */
    unsigned long get_frame_number() const;

    /**
     * Returns number of tracked objects
     * @return Number of tracked objects
     */
    unsigned int get_objects_count() const;

    /**
     * Tracks a new object.
     * @param objectAppearance Object appearance
     * @param objectName Object name
     * @param initialPosition Initial object position
     * @param initialTimestamp Initial timestamp
     * @param initialTimePosition Initial time position
     * @param initialFrameNumber Initial frame number
     * @param endTimestampSet Is end timestamp set?
     * @param endTimestamp End timestamp
     * @param endTimePosition End time position
     * @param endFrameNumber End frame number
     * @return Object ID; First objest has ID 1. If frame at initialTimestamp is not read, -1 is returned;
     */
    int add_object(const Characteristics &objectAppearance, const std::string &objectName,
                   Selection initialPosition, int64_t initialTimestamp, unsigned long initialTimePosition,
                   unsigned long initialFrameNumber, bool endTimestampSet=false, int64_t endTimestamp=0,
                   unsigned long endTimePosition=0, unsigned long endFrameNumber=0);

    /**
     * Reads a frame by its time position and returns it.
     * @param imgFrame Altered read frame (contains tracked objects) converted to be displayed in QT
     * @param originalImgFrame Read frame converted to be displayed in QT.
     * @param includeOriginal Should be original frame read? If not, originalImgFrame does not contain a valid frame.
     * @param time Time position
     * @param progressDialog QT progress dialog for showing an information about tracking objects process
     * @return True if successful
     */
    bool get_frame_by_time(QImage &imgFrame, QImage &originalImgFrame, bool includeOriginal, int64_t time, QProgressDialog *progressDialog);

    /**
     * Reads a frame by its timestamp and returns it.
     * @param imgFrame Altered read frame (contains tracked objects) converted to be displayed in QT
     * @param originalImgFrame Read frame converted to be displayed in QT.
     * @param includeOriginal Should be original frame read? If not, originalImgFrame does not contain a valid frame.
     * @param timestamp Timestamp
     * @param progressDialog QT progress dialog for showing an information about tracking objects process
     * @return True if successful
     */
    bool get_frame_by_timestamp(QImage &imgFrame, QImage &originalImgFrame, bool includeOriginal, int64_t timestamp, QProgressDialog *progressDialog);

     /**
     * Reads a frame by its frame number (index) and returns it.
     * @param imgFrame Altered read frame (contains tracked objects) converted to be displayed in QT
     * @param originalImgFrame Read frame converted to be displayed in QT.
     * @param includeOriginal Should be original frame read? If not, originalImgFrame does not contain a valid frame.
     * @param frameNumber Frame number (index)
     * @param progressDialog QT progress dialog for showing an information about tracking objects process
     * @return True if successful
     */
    bool get_frame_by_number(QImage &imgFrame, QImage &originalImgFrame, bool includeOriginal, unsigned long frameNumber, QProgressDialog *progressDialog);

     /**
     * Reads a frame following to the current one returns it.
     * @param imgFrame Altered read frame (contains tracked objects) converted to be displayed in QT
     * @param originalImgFrame Read frame converted to be displayed in QT.
     * @param includeOriginal Should be original frame read? If not, originalImgFrame does not contain a valid frame.
     * @param progressDialog QT progress dialog for showing an information about tracking objects process
     * @return True if successful
     */
    bool get_next_frame(QImage &imgFrame, QImage &originalImgFrame, bool includeOriginal, QProgressDialog *progressDialog);

    /**
     * Reads a frame preceding to the current one returns it.
     * @param imgFrame Altered read frame (contains tracked objects) converted to be displayed in QT
     * @param originalImgFrame Read frame converted to be displayed in QT.
     * @param includeOriginal Should be original frame read? If not, originalImgFrame does not contain a valid frame.
     * @param progressDialog QT progress dialog for showing an information about tracking objects process
     * @return True if successful
     */
    bool get_previous_frame(QImage &imgFrame, QImage &originalImgFrame, bool includeOriginal, QProgressDialog *progressDialog);

     /**
     * Gets the current frame and returns it.
     * @param imgFrame Altered read frame (contains tracked objects) converted to be displayed in QT
     * @param originalImgFrame Read frame converted to be displayed in QT.
     * @param includeOriginal Should be original frame read? If not, originalImgFrame does not contain a valid frame.
     * @return True if successful
     */
    bool get_current_frame(QImage &imgFrame, QImage &originalImgFrame, bool includeOriginal);

     /**
     * Reads the first frame and returns it.
     * @param imgFrame Altered read frame (contains tracked objects) converted to be displayed in QT
     * @param originalImgFrame Read frame converted to be displayed in QT.
     * @param includeOriginal Should be original frame read? If not, originalImgFrame does not contain a valid frame.
     * @param progressDialog QT progress dialog for showing an information about tracking objects process
     * @return True if successful
     */
    bool get_first_frame(QImage &imgFrame, QImage &originalImgFrame, bool includeOriginal, QProgressDialog *progressDialog);

    /**
     * Creates an output media file including all tracked objects.
     * Throws UserCanceledException when user clicked "Cancel" button in the progress dialog
     * Mind there are two progress dialogs. One is in track_all() and displays infinet progress
     * bar when counting objects positions. The second progress dialog (fileProgressDialog shows percentage of
     * creating the output file. When the first one (in track_all()) is canceled, the second one should be
     * canceled too

     * @param filename Output filename
     * @param fileProgressDialog QT progress dialog for showing an information about creating the file progress
     * @param trackingProgressDialog QT progress dialog for showing an information about tracking objects process
     * @param inFileExtension Input file extensions
     */
    void create_output(std::string const &filename, QProgressDialog &fileProgressDialog,
                       QProgressDialog *trackingProgressDialog, std::string inFileExtension);


    /**
     * Returns appearance of the object.
     * @param objectID Object ID
     * @return Object appearance
     */
    Characteristics get_object_appearance(unsigned int objectID) const;

    /**
     * Returns trajectory sections of the object.
     * @param objectID Object ID
     * @return Trajectory section of the object
     */
    std::map<int64_t, TrajectorySection> const &get_object_trajectory_sections(unsigned int objectID) const;

    /**
     * Returns computed trajectory of the object.
     * @param objectID Object ID
     * @return Trajectory of the object
     */
    std::map<int64_t, TrajectoryEntry> const &get_object_trajectory(unsigned int objectID) const;

    /**
     * Changes appearance of the object.
     * @param objectID Object ID
     * @param newAppearance New appearance of the object
     */
    bool change_object_appearance(unsigned int objectID, Characteristics const &newAppearance);

    /**
     * Rewinds the video to its beginning by seeking its first packet.
     */
    void seek_first_packet();

//    int64_t get_object_end_timestamp(unsigned int objectID, bool &isSet);

    /**
     * Return information about the end of the object.
     * @param objectID Object ID
     * @param timestamp Frame timestamp
     * @param timePosition Time position
     * @param frameNumber Frame number (index)
     * @return True if end is set, false if it is not.
     */
    bool get_object_end(unsigned int objectID, int64_t &timestamp, unsigned long &timePosition, unsigned long &frameNumber);

     /**
     * Changes a trajectory section of the object.
     * @param objectID Object ID
     * @param oldTimestamp Old timestamp of the section
     * @param newTimestamp New timestamp of the section
     * @param position Object position at the first frame of the section
     * @param timePosition Time position of the section
     * @param frameNumber Frame number of the section
     * @return True if successful
     */
    bool change_object_trajectory_section(unsigned int objectID, int64_t oldTimestamp, int64_t newTimestamp, Selection position, unsigned long timePosition, unsigned long frameNumber);

    /**
     * Changes the end frame of the object. If set==false, the end frame will be unset and the object
     * will be tracked till the end of the video. newTimestamp and timePosition are valid onlt if set==true.
     * @param objectID Object ID
     * @param set Set / Unset last frame
     * @param newTimestamp Timestamp of the last frame
     * @param timePosition Time position of the last frame
     * @param frameNumber Frame number of the last frame
     * @return False if given timestamp is lower than the timestamp of Beginning.
     */
    bool change_object_end_frame(unsigned int objectID, bool set, int64_t newTimestamp=0, unsigned long timePosition=0, unsigned long frameNumber=0);

    /**
     * Deletes a trajectory section of the object
     * @param objectID Object ID
     * @param timestamp Trajectory section to be deleted begins at this timestamp
     * @return True if successful
     */
    bool delete_object_trajectory_section(unsigned int objectID, int64_t timestamp);

    /**
     * Sets a trajectory section of the object.
     * @param objectID Object ID
     * @param newTimestamp New timestamp of the section
     * @param position Object position at the first frame of the section
     * @param timePosition Time position of the section
     * @param frameNumber Frame number of the section
     * @return True if successful
     */
    bool set_object_trajectory_section(unsigned int objectID, int64_t newTimestamp, Selection position, unsigned long timePosition, unsigned long frameNumber);

    /**
     * Returns name of the object.
     * @param objectID Object ID
     * @return  Object name
     */
    std::string get_object_name(unsigned int objectID);

    /**
     * Returns names of all objects.
     * @return Names of all objects
     */
    std::vector<std::string> get_all_objects_names();

    /**
     * Sets name of the object.
     * @param objectID Object ID
     * @param newName New object name
     * @return True if successful
     */
    void set_object_name(unsigned int objectID, std::string newName);

    /**
     * Removes the object
     * @param objectID Object ID
     */
    void delete_object(unsigned int objectID);

    /**
     * Computes all trajectory for the tracked object
     * @param objectID ObjectID
     * @param progressDialog QT progress dialog for showing an information about tracking object process
     * @return True if successful
     */
    bool track_object(unsigned int objectID, QProgressDialog *progressDialog);

    /**
     * Erases a part of the computed trajectory. This is necessary after deserialization (with CEREAL)
     * as track_next() initializes correct sections only when the trajectory's last frame is the last
     * frame of the previous section.
     * @param objectID ObjectID
     */
    void erase_object_trajectories_to_comply();

private:
    /**
     * Converts cv::Mat format (OpenCV frame) to QImage (QT)
     * @param src Source frame in cv::Mat
     * @return Frame for QT
     */
    QImage Mat2QImage(cv::Mat const &src) const;

    // previousTimestamp can be used only when previousTimestampSet==true

    /**

     * Draws tracking mark to the result cv::Mat. If current frame was not yet processed, processes tracking till this frame.
     * @param originalFrame Original frame
     * @param result Altered frame; with drawn tracked objects
     * @param progressDialog QT progress dialog for showing an information about tracking objects process
     * @param previousTimestampSet Used only with get_next_frame(). True - this is the following frame to the one given the last time.
     * @param previousTimestamp Timestamp of the preceding frame. This is used only if previousTimestampSet==true.
     * @return Returns true if successful
     */
    bool track_frame(VideoFrame const *originalFrame, cv::Mat &result, QProgressDialog *progressDialog=nullptr, bool previousTimestampSet=false, int64_t previousTimestamp=0);

    /**
     * Tracks all frames. This is called when an output media file is being created.
     * @param progressDialog QT progress dialog for showing an information about tracking objects process
     * @return True if successful
     */
    bool track_all(QProgressDialog *progressDialog);

private:
    QApplication *qApplication; // Is used for updating progress bars
    std::shared_ptr<TrackedObject> a;
    FFmpegPlayer *player;
    VideoFrame *currentFrame;
    VideoFrame *tempFrame;

    std::vector<std::shared_ptr<TrackedObject>> trackedObjects;
};

#endif // VIDEOTRACKER_H
