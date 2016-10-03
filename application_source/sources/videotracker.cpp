/**
 * @file videotracker.cpp
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#include <QImage>
#include <QDebug>

#include "videotracker.h"
#include "avwriter.h"

using namespace cv;

VideoTracker::VideoTracker()
{
    player = nullptr;
    currentFrame = nullptr;
    tempFrame = nullptr;
    qDebug() << "new videoTracker";
}

VideoTracker::VideoTracker(std::string const &videoAddr, QProgressDialog const *progressDialog)
{
    load_video(videoAddr, progressDialog);
}

void VideoTracker::load_video(std::string const &videoAddr, QProgressDialog const *progressDialog)
{
    av_register_all();

    player = new FFmpegPlayer(videoAddr, progressDialog);
    currentFrame = new VideoFrame(player->get_width(), player->get_height()); // stores currently read frame
    tempFrame = new VideoFrame(player->get_width(), player->get_height()); // stores temporary frame when tracking
}

VideoTracker::~VideoTracker()
{

/**
    for (TrackedObject *object: trackedObjects)
    {
        delete object;
        object = nullptr;
    }
*/
    delete currentFrame;
    currentFrame = nullptr;

    delete tempFrame;
    tempFrame = nullptr;

    delete player;
    player = nullptr;
}

void VideoTracker::seek_first_packet()
{

    if (!player->seek_first_packet())
    {
        qDebug() << "ERROR-create_output: seek_first_packet";
    }

}

int VideoTracker::add_object(Characteristics const &objectAppearance, std::string const &objectName, Selection initialPosition,
                             int64_t initialTimestamp, unsigned long initialTimePosition, unsigned long initialFrameNumber,
                             bool endTimestampSet, int64_t endTimestamp, unsigned long endTimePosition, unsigned long endFrameNumber)
{
    qDebug() << "Initialize tracking";

    auto newObject = std::make_shared<TrackedObject>(objectAppearance, objectName, initialTimestamp, initialPosition,
                                                     initialTimePosition, initialFrameNumber, endTimestampSet,
                                                     endTimestamp, endTimePosition, endFrameNumber);

    trackedObjects.push_back(newObject);

    if (!player->get_frame_by_timestamp(currentFrame, initialTimestamp))
    {
        qDebug()<< "Tracker: Cannot read this frame.";
        return -1;
    }

    newObject->track_next(currentFrame);
    qDebug() << "Initialized";

    return trackedObjects.size() - 1;
}

bool VideoTracker::get_frame_by_time(QImage &imgFrame, QImage &originalImgFrame, bool includeOriginal, int64_t time, QProgressDialog *progressDialog)
{

    if (!player->get_frame_by_time(currentFrame, time))
        return false;

    Mat result = currentFrame->get_mat_frame()->clone(); // Simple assignment would do only a shallow copy, clone() is needed
    if (!track_frame(currentFrame, result, progressDialog))
        return false;

    if (includeOriginal)
        originalImgFrame = Mat2QImage(*currentFrame->get_mat_frame());

    imgFrame = Mat2QImage(result);
    return true;
}

bool VideoTracker::get_frame_by_timestamp(QImage &imgFrame, QImage &originalImgFrame, bool includeOriginal, int64_t timestamp, QProgressDialog *progressDialog)
{

    if (!player->get_frame_by_timestamp(currentFrame, timestamp))
        return false;

    Mat result = currentFrame->get_mat_frame()->clone(); // Simple assignment would do only a shallow copy, clone() is needed
    if (!track_frame(currentFrame, result, progressDialog))
        return false;

    if (includeOriginal)
        originalImgFrame = Mat2QImage(*currentFrame->get_mat_frame());

    imgFrame = Mat2QImage(result);
    return true;
}

bool VideoTracker::get_frame_by_number(QImage &imgFrame, QImage &originalImgFrame, bool includeOriginal, unsigned long frameNumber, QProgressDialog *progressDialog)
{

    if (!player->get_frame_by_number(currentFrame, frameNumber))
        return false;

    Mat result = currentFrame->get_mat_frame()->clone(); // Simple assignment would do only a shallow copy, clone() is needed
    if (!track_frame(currentFrame, result, progressDialog))
        return false;

    if (includeOriginal)
        originalImgFrame = Mat2QImage(*currentFrame->get_mat_frame());

    imgFrame = Mat2QImage(result);
    return true;
}

bool VideoTracker::get_next_frame(QImage &imgFrame, QImage &originalImgFrame, bool includeOriginal, QProgressDialog *progressDialog)
{
    //Mat matFrame(player->get_height(), player->get_width(), CV_8UC3);

    // This variable is used for the "track_frame" function to determine whether
    // it needs to jump to a particular frame or if the lastProcessedTimestamp is
    // the same as this variable (previousTimestamp). When it's the same,
    // track_frame does not need to jump since the currentlly read frame is the
    // following to the previously tracked.

    int64_t previousTimestamp = currentFrame->get_timestamp();

    if (!player->get_next_frame(currentFrame))
        return false;

    Mat result = currentFrame->get_mat_frame()->clone(); // Simple assignment would do only a shallow copy, clone() is needed
    if (!track_frame(currentFrame, result, progressDialog, true, previousTimestamp))
        return false;

    if (includeOriginal)
        originalImgFrame = Mat2QImage(*currentFrame->get_mat_frame());

    imgFrame = Mat2QImage(result);
    return true;
}

bool VideoTracker::get_previous_frame(QImage &imgFrame, QImage &originalImgFrame, bool includeOriginal, QProgressDialog *progressDialog)
{
    if (!player->get_previous_frame(currentFrame))
        return false;

    Mat result = currentFrame->get_mat_frame()->clone(); // Simple assignment would do only a shallow copy, clone() is needed
    if (!track_frame(currentFrame, result, progressDialog))
        return false;

    if (includeOriginal)
        originalImgFrame = Mat2QImage(*currentFrame->get_mat_frame());

    imgFrame = Mat2QImage(result);
    return true;
}

bool VideoTracker::get_first_frame(QImage &imgFrame, QImage &originalImgFrame, bool includeOriginal, QProgressDialog *progressDialog)
{
    player->seek_first_packet();

    return get_next_frame(imgFrame, originalImgFrame, includeOriginal, progressDialog);
}


bool VideoTracker::get_current_frame(QImage &imgFrame, QImage &originalImgFrame, bool includeOriginal)
{

    Mat result = currentFrame->get_mat_frame()->clone(); // Simple assignment would do only a shallow copy, clone() is needed
    if (!track_frame(currentFrame, result, nullptr))
        return false;

    if (includeOriginal)
        originalImgFrame = Mat2QImage(*currentFrame->get_mat_frame());

    imgFrame = Mat2QImage(result);
    return true;
}

bool VideoTracker::track_frame(VideoFrame const *originalFrame, cv::Mat &result, QProgressDialog *progressDialog, bool previousTimestampSet,
                               int64_t previousTimestamp)
{
    int64_t currentTimestamp = originalFrame->get_timestamp();

    if (!trackedObjects.empty())
    {
        Selection trackedPosition;
        //for (TrackedObject *object: trackedObjects)
        for (auto object: trackedObjects)
        {
            if ((object->get_initial_timestamp() > currentTimestamp) ||
                    ((object->is_end_timestamp_set() && object->get_end_timestamp() < currentTimestamp)))
                continue; // In this case current timestamp is out of the range of this tracked object

            int64_t lastProcessedTimestamp;
            bool lastProcessedTimestampSet = object->get_last_processed_timestamp(lastProcessedTimestamp); // If the frame was already processed but the whole video is not yet processed
            if (object->is_all_processed() || (lastProcessedTimestampSet && currentTimestamp <= lastProcessedTimestamp))
            {
                if (!object->get_position(currentTimestamp, trackedPosition))// Position is already known as this frame has already been processed.
                {
                    return false;
                }
            }
            else if (previousTimestampSet && (lastProcessedTimestamp == previousTimestamp)) // Is used only with function "get_next_frame"; instead of seeking frame it uses the current one as it is the right one when obtained by get_next_frame(); that's much faster
                trackedPosition = object->track_next(originalFrame);
            /**else if (!object->is_initialized())
            {
                qDebug() << "ERROR - VideoTracker: tracking frame when trackedObject not initialized";
                return false;
            }*/
            else
            {
                if (progressDialog)
                {
                    progressDialog->show();
                }
                if (lastProcessedTimestampSet)
                {
                    // Set to the last processed position. This frame won't be used, but allows to use "get_next_frame".
                    // It might be possible to skip this frame and find right the desired (next) one.
                    // However, it would be more complicated and won't make it much faster.
                    if (!player->get_frame_by_timestamp(tempFrame, lastProcessedTimestamp))
                        return false;

                    if (!player->get_next_frame(tempFrame)) // No more frames => all processed for this object
                    {
                        object->set_all_processed(true);
                        continue;
                    }
                }
                else
                { // So far no frame is processed, therefore lastProcessedTimestamp is not set
                    if (!player->get_frame_by_timestamp(tempFrame, object->get_initial_timestamp()))
                        return false;
                }

                while (true)
                {
                    qApp->processEvents(); // Keeps progress bar active
                    trackedPosition = object->track_next(tempFrame);

                    if (tempFrame->get_timestamp() >= currentTimestamp)
                        break;

                    if (progressDialog)
                    {
                        if (progressDialog->wasCanceled()) // User cancelled the progress dialog
                            throw UserCanceledException();
                    }

                    qApp->processEvents(); // Keeps progress bar active
                    if (!player->get_next_frame(tempFrame))
                        return false; // Could not reach desired frame
                }

                //while (object->get_last_processed_timestamp() < currentTimestamp);

                assert(currentTimestamp == tempFrame->get_timestamp());


                if (progressDialog)
                {
                    if (progressDialog->wasCanceled()) // User cancelled the progress dialog
                        throw UserCanceledException();
                }
            }

            if (!object->draw_mark(result, *originalFrame->get_mat_frame(), currentTimestamp))
                return false;

        }
        //progressDialog->reset();
        return true;
    }

    return true;
}

bool VideoTracker::track_object(unsigned int objectID, QProgressDialog *progressDialog)
{
    auto object = trackedObjects[objectID];
    if (object->is_all_processed())
    {
        return true; // This object is processed throughout all its range
    }
    else
    {
        int64_t endTimestamp = object->get_end_timestamp();

        int64_t lastProcessedTimestamp;
        bool lastProcessedTimestampSet = object->get_last_processed_timestamp(lastProcessedTimestamp); // If the frame was already processed but the whole video is not yet processed

        if (lastProcessedTimestampSet)
        {
            // Set to the last processed position. This frame won't be used, but allows to use "get_next_frame".
            // It might be possible to skip this frame and find right the desired (next) one.
            // However, it would be more complicated and won't make it much faster.
            if (!player->get_frame_by_timestamp(tempFrame, lastProcessedTimestamp))
            {
                //progressDialog->reset();
                return false;
            }

            if (!player->get_next_frame(tempFrame)) // No more frames => all processed for this object
            {
                object->set_all_processed(true);
                return true;
            }
        }
        else
        { // So far no frame is processed, therefore lastProcessedTimestamp is not set
            if (!player->get_frame_by_timestamp(tempFrame, object->get_initial_timestamp()))
            {
                //progressDialog->reset();
                return false;
            }
        }

        while (true)
        {
            qApp->processEvents(); // Keeps progress bar active
            object->track_next(tempFrame); // Also sets object->allProcessed

            if (object->is_end_timestamp_set() && (tempFrame->get_timestamp() >= endTimestamp))
                break;

            if (progressDialog->wasCanceled()) // User canceled the progress dialog
            {
                // reads last tracked frame; is used for knowing where aborted
                player->get_current_frame(currentFrame);
                throw UserCanceledException();
            }

            qApp->processEvents(); // Keeps progress bar active

            if (!player->get_next_frame(tempFrame))
            {
                if (object->is_end_timestamp_set())
                {
                    qDebug() << "Warning-track_all(): Cannot read more frames but object->endTimestamp is higher";
                }
                object->set_all_processed(true);
                break;
            }
        }
    }
    return true;
}


bool VideoTracker::track_all(QProgressDialog *progressDialog)
{
    if (!trackedObjects.empty())
    {
        progressDialog->show();

        unsigned int i;
        for (i = 0; i < trackedObjects.size(); i++)
        {
            if (!track_object(i, progressDialog))
            { // Tracking is not successful, do not track other objects
                qDebug() << "ERROR-track_all(): track_object() returned false";
                return false;
            }
        }

        progressDialog->cancel();
        //progressDialog->reset();
        return true;
    }

    qDebug() << "Warning-track_all(): No objects to track";
    return true;
}

QImage VideoTracker::Mat2QImage(Mat const &src) const
{
    Mat temp; // make the same cv::Mat
    cvtColor(src, temp, CV_BGR2RGB);
    QImage dest((const uchar *) temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888);
    dest.bits(); // enforce deep copy, see documentation
    // of QImage::QImage ( const uchar * data, int width, int height, Format format )
    return dest;
}
/**
bool VideoTracker::set_frame_position(int const &position)
{
      return player->set_frame_position(position);
}
bool VideoTracker::set_time_position(int const &position)
{
      return player->set_time_position(position);
}
*/

double VideoTracker::get_fps() const
{
     return player->get_fps();
}

unsigned long VideoTracker::get_frame_count() const
{
     return player->get_frame_count();
}

unsigned long VideoTracker::get_total_time() const
{
   //  return player->get_frame_count() / player->get_fps();
     return player->get_total_time();
}

int64_t VideoTracker::get_frame_timestamp() const
{
    return currentFrame->get_timestamp();
}

unsigned long VideoTracker::get_time_position() const
{
    return currentFrame->get_time_position();
}

unsigned long VideoTracker::get_frame_number() const
{
    return currentFrame->get_frame_number();
}

unsigned int VideoTracker::get_objects_count() const
{
    return trackedObjects.size();
}

void VideoTracker::create_output(std::string const &filename, QProgressDialog &fileProgressDialog,
                                 QProgressDialog *trackingProgressDialog, std::string inFileExtension)
{
    qDebug() << "filename" <<QString::fromStdString(filename);
    assert(currentFrame != nullptr);

    qDebug() << "Creating output: Tracking started";
    try
    {
        if (!track_all(trackingProgressDialog))
        {
            qDebug() << "ERROR - Creating output: Tracking unsuccessful";
            throw OutputException();
            //return false;
        }
    } catch (UserCanceledException) {
        // Cancel the second progress bar and propagate the Exception
        fileProgressDialog.cancel();
        throw UserCanceledException();
    }
    fileProgressDialog.setValue(0);
    //trackingProgressDialog->cancel();

    qDebug() << "Creating output: Tracking successful";


    qDebug() << "Creating output: Writer initialization";

    AVWriter *writer = new AVWriter();

    if (!writer->initialize_output(filename, player->get_video_stream(), player->get_audio_stream(),
                                   player->get_format_name(), inFileExtension))
    {
        //qDebug() << "Error: Writer not correctly initialized";
        throw OutputException();
        //return false;
    }

    qDebug() << "Creating output: Writer successfully initialized";

    AVPacket packet;
    bool isAudio = false;
    //int i = 0;

    qDebug() << "Creating output: Seeking first packet";

    if (!player->seek_first_packet())
    {
        qDebug() << "ERROR-create_output: seek_first_packet";
        throw OutputException();
        //return false;
    }


    while (player->read_frame(packet, false/*onlyVideoPackets*/, &isAudio))
    { //packet needs to be freed only when contains audio, otherwise is freed already in read_frame()

        if (isAudio)
        {
            if (!writer->write_audio_packet(packet))
            {
                av_free_packet(&packet);
                throw OutputException();
            }
            else
                av_free_packet(&packet);
        }
        else
        {
            if (!player->get_current_frame(currentFrame))
            {
                qDebug() << "Error: creating output: cannot get frame.";
                throw OutputException();
            }


           // Mat result = currentFrame->get_mat_frame()->clone(); // Simple assignment would do only a shallow copy, clone() is needed
            VideoFrame result (*currentFrame); // Simple assignment would do only a shallow copy, clone() is needed

            // All trajectories are counted with track_all();
            // track_frame now only returns those previously counted trajectories.
            if (!track_frame(currentFrame, *(result.matFrame)))
                break;

            if (!writer->write_video_frame(result))
            {
                qDebug() << "Error: write_video_frame";
                throw OutputException();
            }

            qDebug() << currentFrame->get_time_position() << "/" << get_total_time();
            fileProgressDialog.setValue(currentFrame->get_time_position());
            if (fileProgressDialog.wasCanceled())
            { // todo: close file or delete?
                writer->close_output();
                delete writer;
                throw UserCanceledException();
            }
        }

    }
    if (!writer->write_last_frames()) // There might be frames left in the buffer
        qDebug() << "WARNING: Creating output: write_last_frames() not correct";

    qDebug() << "Creating output: Frames successfully written";

    qDebug() << "Creating output: Closing output file";

    if (!writer->close_output())
    {
        qDebug() << "Error: Writer not correctly closed";
        throw OutputException();
        //return false;
    }

   // fileProgressDialog.setValue(fileProgressDialog.maximum()); // In case video was shorter than the claimed length
    delete writer;
    writer = nullptr;

    player->seek_first_packet(); // Return video to the first position
    qDebug() << "Creating output: Output file successfully closed";
    //return true;
}

Characteristics VideoTracker::get_object_appearance(unsigned int objectID) const
{
    return trackedObjects[objectID]->get_appearance();
}

std::map<int64_t, TrajectorySection> const &VideoTracker::get_object_trajectory_sections(unsigned int objectID) const
{
    return trackedObjects[objectID]->get_trajectory_sections();
}

std::map<int64_t, TrajectoryEntry> const &VideoTracker::get_object_trajectory(unsigned int objectID) const
{
    return trackedObjects[objectID]->get_trajectory();
}

bool VideoTracker::set_object_trajectory_section(unsigned int objectID, int64_t newTimestamp, Selection position,
                                                 unsigned long timePosition, unsigned long frameNumber)
{
    return trackedObjects[objectID]->set_trajectory_section(newTimestamp, position, timePosition, frameNumber); //todo check if exists
}

bool VideoTracker::change_object_trajectory_section(unsigned int objectID, int64_t oldTimestamp, int64_t newTimestamp,
                                                    Selection position, unsigned long timePosition, unsigned long frameNumber)
{
    return trackedObjects[objectID]->change_trajectory_section(oldTimestamp, newTimestamp, position, timePosition, frameNumber); //todo check if exists
}

bool VideoTracker::delete_object_trajectory_section(unsigned int objectID, int64_t timestamp)
{
    return trackedObjects[objectID]->delete_trajectory_section(timestamp); //todo check if exists
}

bool VideoTracker::change_object_end_frame(unsigned int objectID, bool set, int64_t timestamp,
                                           unsigned long timePosition, unsigned long frameNumber)
{
    return trackedObjects[objectID]->change_end_frame(set, timestamp, timePosition, frameNumber); //todo test if object exists
}

bool VideoTracker::get_object_end(unsigned int objectID, int64_t &timestamp, unsigned long &timePosition, unsigned long &frameNumber)
{
    if (!trackedObjects[objectID]->is_end_timestamp_set())
        return false;

    timestamp = trackedObjects[objectID]->get_end_timestamp();
    timePosition = trackedObjects[objectID]->get_end_time_position();
    frameNumber = trackedObjects[objectID]->get_end_frame_number();
    return true;
}

bool VideoTracker::change_object_appearance(unsigned int objectID, Characteristics const &newAppearance)
{
    trackedObjects[objectID]->change_appearance(newAppearance);
    return true;
}

std::string VideoTracker::get_object_name(unsigned int objectID)
{
    return trackedObjects[objectID]->get_name();
}

void VideoTracker::set_object_name(unsigned int objectID, std::string newName)
{
    trackedObjects[objectID]->set_name(newName);
}

void VideoTracker::delete_object(unsigned int objectID)
{
    assert(trackedObjects.size() > objectID);
    trackedObjects.erase(trackedObjects.begin()+objectID);
}

std::vector<std::string> VideoTracker::get_all_objects_names()
{
    std::vector<std::string> objectNames;

    for (unsigned int i = 0; i < trackedObjects.size(); i++)
        objectNames.push_back(get_object_name(i));

     return objectNames;
}

void VideoTracker::erase_object_trajectories_to_comply()
{
    for (auto object: trackedObjects)
    {
        object->erase_trajectory_to_comply();
    }
}
