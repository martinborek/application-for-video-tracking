/**
 * @file mainwindow.h
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileInfo>
#include <QListWidgetItem>
#include <QSettings>
#include <QHelpEngine>

#include "imagelabel.h"
#include "timelabel.h"
#include "videotracker.h"
#include "anchoritem.h"
#include "trajectoryitem.h"

// SERIALIZATION WITH CEREAL
#include <cereal/types/string.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/utility.hpp> // for std::pair

enum class SelectionState : int {NO_SELECTION, NEW_OBJECT, CHANGE_POSITION,
                                 SET_END_FRAME, NEW_SECTION};

namespace Ui {
    class MainWindow;
}
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * CEREAL serialization
     */
    template<class Archive>
    void serialize(Archive &archive)
    {
        archive(CEREAL_NVP(inputFileName), CEREAL_NVP(tracker), CEREAL_NVP(customColorsCount), cereal::make_nvp("colors", colorsMap));
    }

    /**
     * Constructor
     * @param parent Parent widget
     */
    explicit MainWindow(QWidget *parent = 0);

     /**
     * Destructor
     */
    ~MainWindow();
    
private:
    /**
     * Asks for saving a project before closing the application.
     * @param event Event
     */
    void closeEvent(QCloseEvent *event);

    /**
     * Connects signals and slots.
     */
    void connect_signals();

    /**
     * Sets the GUI to a state when the end of the video was reached.
     */
    void end_of_video();

    /**
     * When a frame is showed, slider value and time are changed. Used only as an internal function,
     * that is called by other functions (show_next_frame, show_previous_frame ...) that are reading
     * correct frames.
     */
    void show_frame();

    /**
     * Sets values of Appearance forms.
     */
    void set_form_values();

    /**
     * This is used when appearance of the object has changed. The VideoTracker needs to be informed
     * to register these changes.
     */
    void change_appearance();

    /**
     * Fills the Trajectory tab with values.
     * @param id Object ID
     * @param clear If true, trajectory is cleared and filled again.
     * @param showProgressBar If true, shows progress bar.
     */
    void set_trajectory_tab(unsigned int id, bool clear=false, bool showProgressBar=true);

    /**
     * Fills the Key points tab with values.
     * @param id Object ID
     */
    void set_anchors_tab(unsigned int id);

    /**
     * Fills the Appearance tab with values.
     * @param id Object ID
     */
    void set_appearance_tab(unsigned int id);

    /**
     * Restores default values of the Appearance tab.
     */
    void show_appearance_tab();

    /**
     * Shows a frame by a given timestamp.
     * @param timestamp Timestamp
     */
    void show_frame_by_timestamp(int64_t timestamp);


    /**
     * Confirms adding a new tracked object.
     * @param selectedPosition Position of the new object
     */
    void new_object_confirm(Selection const &selectedPosition);

    /**
     * Confirms position change.
     * @param selectedPosition New position
     * @return True if successful
     */
    bool change_position_confirm(Selection const &selectedPosition);

    /**
     * Confirms the last frame selection of the object
     * @return True if successful
     */
    bool set_end_frame_confirm();

    /**
     * Confirms new trajectory change (correction).
     * @return True if successful
     */
    bool new_section_confirm(const Selection &selectedPosition);

    /**
     * Disables everything apart from the video player, shows selectionWidget.
     * @param state Selected option
     * @param enable_selecting If true, enables selecting.
     */
    void set_selection(SelectionState state, bool enable_selecting=true);

    /**
     * Asks a user for an object name. The new object name is stored in newObjectName.
     * @param change
     * @return True if name entered correctly. False if user clicked Cancel.
     */
    bool enter_object_name(bool change);

    /**
     * Sets ui->objectsBox to display tracked objects that exist in the tracker.
     * @param noTabsUpdate If true, do not change tabs according current object's index.
     * This is used when an object is selected right after calling this function
     * and setting tabs in this function would be useless. This saves the time
     * that would be used to setting the tabs (especially the Trajectory tab
     * takes a long time to be set)
     */
    void show_objects_box(bool noTabsUpdate=false);

    /**
     * Sets the main application menu.Needs to be called AFTER enabling/disabling widget (that affect
     * buttons).* It can be called only once whe changing a group of buttons, but always AFTER all
     * changes are made. Sets also Window's title.
     */
    void set_application_menu();

    /**
     * When appearance changes were made, should they be confirmed or discarded?
     * @return True if changes were confirmed, discarded or if no changes were made. False if user
     * pressed "Cancel" button and nothing is done with appearance changes.
     */
    bool ask_save_appearance_changes();

    /**
     * When project is being closed, should it be saved?
     * @return True if project was saved or not. False if "Cancel" button was pressed.
     */
    bool ask_save_project();

    /**
     * Restores the initial application settings.
     */
    void initial_application_settings();

    /**
     * Pauses the video playback.
     */
    void pause();

    /**
     * Shows the first frame of the video.
     */
    void show_first_frame();

    /**
     * An internal function used when changing playback speed.
     */
    void speed_general();

    /**
     * Shows a dilog for opening a video.
     * Result is stored in inputFileName
     * @return False if Cancel button pressed. True otherwise.
     */
    bool open_video_dialog();

    /**
     * Sets the application to display successfully opened video
     */
    void open_video_successful();

    /**
     * Lets user select object color by a color picker.
     */

    void custom_color();
    /**
     * Lets user select border object color by a color picker.
     */
    void custom_border_color();

private slots:

    /**
     * Context menu that is displayed when clicking on an item in the Trajectory tab
     * @param item Clicked item
     */
    void show_anchors_menu(QListWidgetItem *item);

    /**
     * When an object is selected, display its settings
     */
    void set_object_settings();

    /**
     * Slider has been pressed.
     */
    void slider_pressed();

    /**
     * Slider has been released.
     */
    void slider_released();

    /**
     * Shows a frame by a given time position.
     * @param position Time position
     */
    void show_frame_by_time(int position);

    /**
     * Sets playback faster.
     */
    void faster();

    /**
     * Sets playback slower.
     */
    void slower();

    /**
     * Shows the following frame.
     */
    void show_next_frame();

    /**
     * Shows the previous frame.
     */
    void show_previous_frame();

    /**
     * Plays / pauses the video.
     */
    void play();

    /**
     * Stops the video
     */
    void stop();

    /**
     * Opens a new video
     */
    void open_video();

    /**
     * Allows users to create a Custom color - select it by a color picker
     * @param newColorID Used for returning the ID of the created color
     * @param initialColor Initial color of the picker
     * @return True if a color selected. False if Cancel button pressed.
     */
    bool color_picker(unsigned int &newColorID, const ObjectColor &initialColor);

    /**
     * Creats an output video file with tracked objects.
     */
    void create_output();

    /**
     * Adds a new object. After receiving an object name sets UI for selecting an area
     * for the TrackingAlgorithm.
     */
    void add_new_object();

    /**
     * Sets GUI to look like before adding new object.
     */
    void selection_end();

    /**
     * Sets a new picture when the label ui->videoLabel is resized.
     */
    void reload_video_label();

    /**
     * Changes a shape of the object.
     */
    void change_shape();

    /**
     * Changes a border thickness of the object.
     */
    void change_border_thickness();

    /**
     * Changes a border color of the object.
     */
    void change_border_color();

    /**
     * Changes a color of the object.
     */

    void change_color();

    /**
     * Changes the object to be filled with a color.
     */
    void change_draw_inside();

    /**
     * Changes the object so that its border is drawn.
     */
    void change_draw_border();

    /**
     * Changes defocusing size (square size) for the object.
     */
    void change_defocus_size();

    /**
     * Changes the object to be defocused.
     */
    void change_defocus();

    /**
     * Discards changes to the object appearance.
     */
    void discard_appearance_changes();

    /**
     * Confirms changes to the object appearance.
     */
    void confirm_appearance_changes();

    /**
     * Area / video position was selected and confirmed.
     * Calls responsible function; either for New object, Position change, Setting
     * object's end frame or New trajectory change
     */
    void selection_confirmed();

    /**
     * Initiates a trajectory change (correction)
     */

    void add_trajectory_change();

    /**
     * Changes name of the object.
     */
    void change_name();

    /**
     * Removed the object.
     */
    void delete_object();

    /**
     * Switches the appliction to Czech.
     */
    void set_czech_language();

    /**
     * Switches the appliction to English.
     */
    void set_english_language();

    /**
     * Sets playback speed to its original value.
     */
    void original_speed();


    /**
     * Sets object tracking till the end of the video.
     */
    void set_video_end();

    /**
     * Sets frame where tracking of the object shall end.
     */
    void set_end_frame();

    /**
     * Changes beginning of the object.
     */
    void set_change_beginning();

    /**
     * Shows the previous frame
     */
    void step_back();

    /**
     * Shows the following frame
     */
    void step_forward();

    /**
     * Save the project in XML or JSON.
     * @return True if successful.
     */
    bool save_project();

    /**
     * Loads a project in XML or JSON.
     */
    void load_project();

    /**
     * Shows a frame by a given number (index)
     * @param position Number (index)
     */
    void show_frame_by_number(unsigned long position);

    /**
     * Displays time positions.
     */
    void display_time();

    /**
     * Displays frame numbers.
     */
    void display_frame_numbers();

    /**
     * Sets / unsets showing the original video beside the altered one.
     */
    void show_original_video();

    /**
     * Computes all trajectory for the object
     */
    void compute_trajectory();

    /**
     * Shows frame of the selected trajectory item.
     * @param item Selected item
     */
    void trajectory_show_frame(QListWidgetItem *item);

    /**
     * Shows application help.
     */
    void show_help();

    /**
     * Shows information about the application.
     */
    void show_about();

private:
    const QString applicationName;
    Ui::MainWindow *ui;
    QHelpEngine *helpEngine;
    QSettings *settings;
    bool displayTime; //false ~ show frame numbers
    bool showOriginalVideo;

    QImage frame;
    QImage originalFrame;
    std::unique_ptr<VideoTracker> tracker;
    QTimer * timer;
    unsigned int timerInterval;
    int timerSpeed;

    SelectionState selectionState;
    QString newObjectName;
    AnchorItem *editingAnchorItem; // When show_anchors_menu() called, this variable is set to know what item is being edited

    bool isPlaying;
    bool endOfVideo;
    bool appearanceChanged;
    bool projectChanged; // to ask if user wants to save it
    bool settingObjectSettings;
    bool isEndTimestampSet; // used with AnchorItem - determining whether to show option "Track till the end of the video"
    std::string inputFileName;

    // Last loaded/saved project; FileDialog starts with at this path.
    QString projectFileName;

    // Currently opened project to be displayed in the window's title
    QString projectName;

    std::map<unsigned int, std::pair<std::string, ObjectColor>> colorsMap;
    unsigned int customColorsCount;
    std::map<unsigned int, QString> shapesMap;

    Characteristics originalObjectAppearance;
    Characteristics alteredObjectAppearance;

    QString computingInfoText;
    QString computingInfoTitle;

};

#endif // MAINWINDOW_H
