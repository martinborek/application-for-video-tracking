/**
 * @file mainwindow.cpp
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "selection.h"
#include "colors.h"
#include "objectshape.h"

#include <cereal/archives/json.hpp>
#include <cereal/archives/xml.hpp>
#include <utility> // for std::pair
#include <fstream> // for cereal output

#include <QtGui>
#include <QDebug>
#include <QFileDialog>
#include <QPainter>
#include <QProgressDialog>
#include <QInputDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QMenu>
#include <QTranslator>
#include <QSplitter>
#include <QDockWidget>
#include <helpbrowser.h>

#define VIDEOTRACKING_10MS2S 100 // 1 second is 100*10 milliseconds
#define VIDEOTRACKING_TIMER_CONSTANT 1.5
#define VIDEOTRACKING_TIMER_CONSTANT_FAST 0.5
#define VIDEOTRACKING_TIMER_CONSTANT_SLOW 0.2

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    applicationName("Video Anonymizer"),
    ui(new Ui::MainWindow)
{

    settings = new QSettings("FIT", applicationName, this); // application and company name
    qApp->setApplicationName(applicationName);
    this->setWindowTitle(applicationName);

    // TRANSLATOR - BEGINNING
    QTranslator * translator = new QTranslator(this);
    bool checkEnglish = false; // To find out what language should be checked in application menu

    if (settings->contains("language"))
    {   // load language

        qDebug() << "Language loaded";

        QString languageFile = settings->value("language").toString();
        translator->load(languageFile);

        if (languageFile.isEmpty()) // Empty string means default language (English) is set
            checkEnglish = true;
    }
    else
    {// set language according locale

        qDebug() << "Language set according locale";

        QString locale = QLocale::system().name();
        if (!translator->load(QString("video_anonymizer_" + locale)))
            checkEnglish = true; // Loading of language was not succesful, check English (default) option
        else // English is default
            checkEnglish = false;
    }


    // apply requested language
    // must be done before setting ui (ui->setupUi) and setting any text in widgets
    qApp->installTranslator(translator);
    // TRANSLATOR - END

    ui->setupUi(this);

    // Set language menu
    if (checkEnglish)
        ui->actionEnglish->setChecked(true);
    else
        ui->actionCzech->setChecked(true);

    tracker = nullptr; // Will be initialized when new video is loaded
    timer = new QTimer(this); //Is set when play button clicked

    // Should original video be showed?
    if (settings->contains("showOriginalVideo"))
    { // Is showOriginalVideo variable set in settings?

        showOriginalVideo = settings->value("showOriginalVideo").toBool();
    }
    else
    {   // Not set => set default value
        showOriginalVideo = true;
        settings->setValue("showOriginalVideo", showOriginalVideo);
    }
    ui->originalVideoFrame->setVisible(showOriginalVideo);

    // Display time / frame numbers
    if (settings->contains("showTime"))
    { // Is displayTime variable set in settings?
        qDebug() << "Time/Frame num loaded";

        displayTime = settings->value("showTime").toBool();
    }
    else
    {
        displayTime = true;
        settings->setValue("showTime", displayTime); // Not set => set default
    }

    initial_application_settings();
#
    computingInfoText = tr("Computing positions of tracked object");
    computingInfoTitle = tr("Computing");

    //progressDialog = new QProgressDialog(tr("Tracking"), tr("Cancel"), 0, 0, this);
    //progressDialog->setModal(true);

    // Setup help data:
    //helpEngine = new QHelpEngine("help/help_cs.qhc", this);

    QString helpDir(QCoreApplication::applicationDirPath() +  "/" + tr("help_en.qhc"));
    qDebug() <<  "help:" << helpDir;
    helpEngine = new QHelpEngine(helpDir, this);
    if (!helpEngine->setupData())
    {
        qDebug() << "Help not loaded successfully";
        qDebug() << helpEngine->error();
    }
    else
        qDebug() << "Help loaded";
    //helpEngine = new QHelpEngine(+tr("help_en"), this):
   //qDebug() << helpEngine->error();


    connect_signals(); // Connects GUI elements with slots
}


MainWindow::~MainWindow()
{
    timer->stop(); // "timer" is deleted automatically; stopping is enough

    //delete tracker; // is smart pointer => will be deleted automatically
    delete ui;

    qDebug() << "Destruct: MainWindow destroyed";
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!ask_save_project())
        event->ignore();
    else
        event->accept();
}

/** Using reload_video_label() instead
void MainWindow::resizeEvent(QResizeEvent *)
{
    playerWidget->setGeometry(0, 0, ui->videoWidget->width(), ui->videoWidget->height());
    ui->videoLabel->setGeometry(0, 0, ui->videoFrame->width(), ui->videoFrame->height());
    timeLabel->setGeometry(0, 0, ui->timeWidget->width(), ui->timeWidget->height());
}
*/

void MainWindow::reload_video_label()
{
    // make size of originalVideoFrame fixed and videoFrame will expand
    ui->originalVideoFrame->setFixedWidth(ui->videosLayout->geometry().width() / 2.5);

    if (tracker)
    {
        ui->videoLabel->set_image(frame, ui->videoFrame->width(), ui->videoFrame->height());

        if (showOriginalVideo)
            ui->originalVideoLabel->set_image(originalFrame, ui->originalVideoFrame->width(), ui->originalVideoFrame->height());
    }

}

void MainWindow::pause()
{
    isPlaying = false;
    ui->playButton->setText(tr("Play"));
    timer->stop();

}

/* Play / Pause */
void MainWindow::play()
{


    if (isPlaying)
        pause();
    else
    {
        if (endOfVideo) // End of video => Start from beginning
            show_first_frame();

        isPlaying = true;

        ui->playButton->setText(tr("Pause"));


        if (!tracker)
        {
            qDebug() << "WARNING: play() called with no video loaded";
            return;
        }

        timer->start(); // Displays new frames in given interval.
      }
}

/* Stop playing, return to first frame and display it */
void MainWindow::stop()
{
    isPlaying = false;

    timer->stop();

    ui->playButton->setText(tr("Play"));
    ui->stepBackButton->setEnabled(true);
    ui->positionSlider->setEnabled(true);
    set_application_menu();

    show_first_frame();
}

/* Select a file and open it */
void MainWindow::open_video()
{
    if (tracker)
    { // Project exists, ask to save/discard
        if (!ask_save_project())
            return; // User canceled the dialog, do not open a new video

        initial_application_settings(); // Restores default application settings

    }

    if (!open_video_dialog())
        return; // Cancel button pressed

    QProgressDialog progressDialog(tr("Opening video"), tr("Cancel"), 0, 0, this);
    progressDialog.setWindowTitle(tr("Opening"));
    progressDialog.setModal(true);
    progressDialog.show();

    try
    {
        tracker = std::unique_ptr<VideoTracker>(new VideoTracker(inputFileName, &progressDialog));

    } catch (OpenException) {

        QMessageBox *alert = new QMessageBox(this);
        alert->setWindowTitle(tr("Opening video"));
        alert->setText(tr("This video file cannot be opened."));
        alert->exec();

        return;

    } catch (UserCanceledOpeningException) {

        QMessageBox *alert = new QMessageBox(this);
        alert->setWindowTitle(tr("Opening video"));
        alert->setText(tr("Video opening was canceled."));
        alert->exec();

        return;
    }
    //progressDialog->reset();
    open_video_successful();
}

bool MainWindow::open_video_dialog()
{
    QFileDialog fileDialog(this);
    fileDialog.setWindowTitle(tr("Open Video"));

    if (settings->contains("lastPath"))
    { // begin with the directory where the last video was opened
        QString lastPath = settings->value("lastPath").toString();
        fileDialog.setDirectory(lastPath);
    }

    QStringList nameFilters;

    // List of abbreviations considered to belong to video files
    nameFilters << tr("Video files") + " (*.3g2 *.3gp *.asf *.avi *.flv *.m2v *.m4p *.m4v *.mkv *.mov *.mp2 *.mp4 "
                   "*.mpe *.mpeg *.mpg *.mpv *.ogg *.ogv *.qt *.rm *.rmvb *.vob *.webm *.wmv *.yuv)";
    nameFilters << tr("All files") + " (*)";

    fileDialog.setNameFilterDetailsVisible(false);
    //fileDialog.setNameFilterDetailsVisible(true); //there is too many abbreviations to show
    fileDialog.setNameFilters(nameFilters);

    fileDialog.setFileMode(QFileDialog::ExistingFile);
    fileDialog.setViewMode(QFileDialog::Detail);

    if (fileDialog.exec() && !fileDialog.selectedFiles().isEmpty())
    {
        // save directory path for next opening:

        inputFileName = fileDialog.selectedFiles().takeFirst().toStdString();

        QString directoryPath = fileDialog.directory().absolutePath();

        settings->setValue("lastPath", directoryPath);

        return true;
    }
    else
        return false; // Cancel button pressed
}

void MainWindow::open_video_successful()
{
    qDebug() << "Video correctly loaded";
    show_appearance_tab();

    endOfVideo = false;

    timerInterval = 1000 / (tracker->get_fps());
    timer->setInterval(timerInterval);

    ui->timeLabel->set_total_time(tracker->get_total_time());
    ui->timeLabel->set_frame_count(tracker->get_frame_count());

    if (displayTime)
        ui->timeLabel->display_time(0); // First frame is at time 0
    else
        ui->timeLabel->display_frame_num(1); // First frame is at number 1

    ui->positionSlider->setMinimum(1);

    //ui->positionSlider->setMaximum(tracker->get_total_time()/VIDEOTRACKING_10MS2S); // value is in 0.1s
    ui->positionSlider->setMaximum(tracker->get_frame_count()); // value is in 0.1s

    // Disable slider steps. Jump straight to pressed position instead (signal pressed_position);
    ui->positionSlider->setSingleStep(0);
    ui->positionSlider->setPageStep(0);

    ui->positionSlider->setEnabled(true);
    ui->playerControlsWidget->setEnabled(true);

    //ui->createOutputButton->setEnabled(true);
    ui->actionCreateOutput->setEnabled(true);

    ui->objectsControlWidget->setEnabled(true);

    ui->originalVideoTextLabel->setVisible(true);

    set_application_menu();
    show_next_frame(); // Displays first frame
}

void MainWindow::end_of_video()
{
    qDebug() << "End of video.";
    //tracker->set_frame_position(0);
    //endOfVideo = true;
    isPlaying = false;

    ui->playButton->setText(tr("Play"));
    ui->stepBackButton->setEnabled(true);
    ui->positionSlider->setEnabled(true);

    timer->stop();

    set_application_menu();
}

void MainWindow::step_back()
{
    pause();
    show_previous_frame();
}

void MainWindow::step_forward()
{
    pause();
    show_next_frame();
}

void MainWindow::show_frame()
{
    if (tracker->get_frame_number() == tracker->get_frame_count())
        endOfVideo = true;
    else
        endOfVideo = false;

    //ui->positionSlider->setValue(tracker->get_time_position()/VIDEOTRACKING_10MS2S);
    ui->positionSlider->setValue(tracker->get_frame_number());

    if (displayTime)
        ui->timeLabel->display_time(tracker->get_time_position());
    else
        ui->timeLabel->display_frame_num(tracker->get_frame_number());

    //qDebug() << "frame number" << tracker->get_frame_number();

    ui->videoLabel->set_image(frame, ui->videoFrame->width(), ui->videoFrame->height());

    if (showOriginalVideo)
        ui->originalVideoLabel->set_image(originalFrame, ui->originalVideoFrame->width(), ui->originalVideoFrame->height());

    if (tracker->get_objects_count())
        set_trajectory_tab(ui->objectsBox->currentData().toUInt(), false, false);

    //qDebug()<< "slider position: " << ui->positionSlider->value();
}

void MainWindow::show_next_frame()
{
    // Save timestamp to be restored if operation is aborted
    int64_t currentTimestamp = tracker->get_frame_timestamp();

    QProgressDialog progressDialog(computingInfoText, tr("Cancel"), 0, 0, this);
    progressDialog.setWindowTitle(computingInfoTitle);
    progressDialog.setModal(true);
    try
    {
        if (!tracker->get_next_frame(frame, originalFrame, showOriginalVideo, &progressDialog))
        {
            qDebug() << "No next frame to show";
            end_of_video(); // Frame was not read successfully -> end of video
        }
        else
            show_frame();
    } catch (UserCanceledException)
    {
        progressDialog.cancel();
        show_frame_by_timestamp(currentTimestamp); // restore player position before the computing started
        return;
    }
}

void MainWindow::show_previous_frame()
{
    // Save timestamp to be restored if operation is aborted
    int64_t currentTimestamp = tracker->get_frame_timestamp();

    QProgressDialog progressDialog(computingInfoText, tr("Cancel"), 0, 0, this);
    progressDialog.setWindowTitle(computingInfoTitle);
    progressDialog.setModal(true);
    try
    {
        if (!tracker->get_previous_frame(frame, originalFrame, showOriginalVideo, &progressDialog))
            qDebug() << "No previous frame to show";
        else
            show_frame();
    } catch (UserCanceledException)
    {
        progressDialog.cancel();
        show_frame_by_timestamp(currentTimestamp); // restore player position before the computing started
        return;
    }
}

void MainWindow::show_frame_by_time(int position)
{
    // Save timestamp to be restored if operation is aborted
    int64_t currentTimestamp = tracker->get_frame_timestamp();

    QProgressDialog progressDialog(computingInfoText, tr("Cancel"), 0, 0, this);
    progressDialog.setWindowTitle(computingInfoTitle);
    progressDialog.setModal(true);
    try
    {
        if (!tracker->get_frame_by_time(frame, originalFrame, showOriginalVideo,
                                        position*VIDEOTRACKING_10MS2S, &progressDialog))
        {
            qDebug() << "WARNING: Cannot read frame at given time position";
            end_of_video();
        }
        else
            show_frame();
    } catch (UserCanceledException)
    {
        progressDialog.cancel();
        show_frame_by_timestamp(currentTimestamp); // restore player position before the computing started
        //progressDialog->reset();
        return;
    }
}

void MainWindow::show_frame_by_timestamp(int64_t timestamp)
{
    // Save timestamp to be restored if operation is aborted
    int64_t currentTimestamp = tracker->get_frame_timestamp();

    QProgressDialog progressDialog(computingInfoText, tr("Cancel"), 0, 0, this);
    progressDialog.setWindowTitle(computingInfoTitle);
    progressDialog.setModal(true);
    try
    {
        if (!tracker->get_frame_by_timestamp(frame, originalFrame, showOriginalVideo, timestamp, &progressDialog))
        {
            qDebug() << "ERROR: show_frame_by_timestamp() - cannot read frame";
            end_of_video();
        }
        else
            show_frame();
    } catch (UserCanceledException)
    {
        progressDialog.cancel();
        show_frame_by_timestamp(currentTimestamp); // restore player position before the computing started
        //progressDialog->reset();
        return;
    }
}

void MainWindow::show_frame_by_number(unsigned long position)
{
    // Save timestamp to be restored if operation is aborted
    int64_t currentTimestamp = tracker->get_frame_timestamp();

    QProgressDialog progressDialog(computingInfoText, tr("Cancel"), 0, 0, this);
    progressDialog.setWindowTitle(computingInfoTitle);
    progressDialog.setModal(true);
    try
    {
        if (!tracker->get_frame_by_number(frame, originalFrame, showOriginalVideo, position, &progressDialog))
        {
            qDebug() << "WARNING: Cannot read frame at given number position";
            end_of_video();
        }
        else
            show_frame();
    } catch (UserCanceledException)
    {
        progressDialog.cancel();
        show_frame_by_timestamp(currentTimestamp); // restore player position before the computing started
        //progressDialog->reset();
        return;
    }
}

void MainWindow::show_first_frame()
{
    // Save timestamp to be restored if operation is aborted
    int64_t currentTimestamp = tracker->get_frame_timestamp();

    QProgressDialog progressDialog(computingInfoText, tr("Cancel"), 0, 0, this);
    progressDialog.setWindowTitle(computingInfoTitle);
    progressDialog.setModal(true);
    try
    {
        if (!tracker->get_first_frame(frame, originalFrame, showOriginalVideo, &progressDialog))
        {
            qDebug() << "WARNING: Cannot get first frame";
            end_of_video();
        }
        else
            show_frame();
    } catch (UserCanceledException)
    {
        progressDialog.cancel();
        show_frame_by_timestamp(currentTimestamp); // restore player position before the computing started
        //progressDialog->reset();
        return;
    }

}

void MainWindow::faster()
{
    timerSpeed++;
    speed_general();
}

void MainWindow::slower()
{
    if (timerSpeed <= -4)
        return; // Cannot make it slower

    timerSpeed--;
    speed_general();
}

void MainWindow::original_speed()
{
    timerSpeed = 0;
    speed_general();
}

void MainWindow::speed_general()
{
    double speed = 1;

    if (timerSpeed > 0)
        speed += timerSpeed*VIDEOTRACKING_TIMER_CONSTANT_FAST;
    else if (timerSpeed < 0)
        speed += timerSpeed*VIDEOTRACKING_TIMER_CONSTANT_SLOW;

    timer->setInterval(timerInterval / speed);
    ui->speedLabel->setText(tr("Speed:") + " " + QString::number(speed, 'f', 2) + "x");
}

void MainWindow::create_output()
{
    pause();

    // Save timestamp to restore it after the operation is complete
    int64_t currentTimestamp = tracker->get_frame_timestamp();

    if (!ask_save_appearance_changes())
        return;

    std::string input = inputFileName;
    auto extensionPosition = input.find_last_of('.');

    std::string defaultFileName;
    if (extensionPosition != std::string::npos)
    { // extension is present in the filename
        defaultFileName = input.substr(0, extensionPosition) + "_output";
        defaultFileName += input.substr(extensionPosition);
    }
    else
        defaultFileName = input + "_output";

    QString filename = QFileDialog::getSaveFileName(this, tr("Select output file"),
                                                    QString::fromStdString(defaultFileName));
    if (filename.isEmpty())
        return;

   // qApp->processEvents();

    qDebug() << "Creating output started";
    QProgressDialog fileProgressDialog(tr("Creating output file..."), tr("Cancel"), 0, tracker->get_total_time(), this);
    fileProgressDialog.setWindowTitle(tr("Creating Output"));
    fileProgressDialog.setWindowModality(Qt::WindowModal);
    //fileProgressDialog.setWindowFlags(Qt::Window | Qt::);
    fileProgressDialog.setAutoReset(false);
    fileProgressDialog.setMinimumDuration(0); // Always show the dialog; Otherwise it would be shower only if process would take at least certain time

    QProgressDialog trackingProgressDialog(tr("Computing trajectories of all tracked objects"), tr("Cancel"), 0, 0, this);
    fileProgressDialog.setWindowTitle(computingInfoTitle);
    trackingProgressDialog.setModal(true);
    //trackingProgressDialog.show();
    QMessageBox *alert = new QMessageBox(this);
    alert->setWindowTitle(tr("Creating Output"));

    std::string inFileExtension = "";
    auto inExtensionPosition  = inputFileName.find_last_of('.');
    if (inExtensionPosition != std::string::npos)
         inFileExtension = inputFileName.substr(inExtensionPosition);

    try
    {
        tracker->create_output(filename.toStdString(), fileProgressDialog, &trackingProgressDialog, inFileExtension);
        alert->setText(tr("Output successfully created"));
    } catch (OutputException) {
        qDebug() << "ERROR: Output creating not successful";
        alert->setText(tr("Error occured when creating the output file. Output was not created."));
    } catch (UserCanceledException) {
        qDebug() << "User canceled output creation";
        alert->setText(tr("Creating output file was canceled. Output was not created."));
    }

    fileProgressDialog.cancel();

    alert->exec();

    show_frame_by_timestamp(currentTimestamp); // restore player position before the computing started
}

void MainWindow::slider_pressed()
{
    timer->stop();
}

void MainWindow::slider_released()
{
    //show_frame_by_number(ui->positionSlider->value());

    if (isPlaying)
    {
        timer->start();
    }
}

void MainWindow::set_form_values()
{
    colorsMap[Colors::BLACK] = std::make_pair(tr("Black").toStdString(), ObjectColor(0, 0, 0));
    colorsMap[Colors::GRAY] = std::make_pair(tr("Gray").toStdString(), ObjectColor(128, 128, 128));
    colorsMap[Colors::SILVER] = std::make_pair(tr("Silver").toStdString(), ObjectColor(192, 192, 192));
    colorsMap[Colors::WHITE] = std::make_pair(tr("White").toStdString(), ObjectColor(255, 255, 255));
    colorsMap[Colors::RED] = std::make_pair(tr("Red").toStdString(), ObjectColor(255, 0, 0));
    colorsMap[Colors::GREEN] = std::make_pair(tr("Green").toStdString(), ObjectColor(0, 255, 0));
    colorsMap[Colors::BLUE] = std::make_pair(tr("Blue").toStdString(), ObjectColor(0, 0, 255));
    colorsMap[Colors::YELLOW] = std::make_pair(tr("Yellow").toStdString(), ObjectColor(255, 255, 0));
    colorsMap[Colors::CYAN] = std::make_pair(tr("Cyan").toStdString(), ObjectColor(0, 255, 255));
    colorsMap[Colors::MAGENTA] = std::make_pair(tr("Magenta").toStdString(), ObjectColor(255, 0, 255));

    shapesMap.clear();
    shapesMap[ObjectShape::RECTANGLE] = tr("Rectangle");
    shapesMap[ObjectShape::ELLIPSE] = tr("Ellipse");
}

void MainWindow::show_anchors_menu(QListWidgetItem *item)
{
    editingAnchorItem = static_cast<AnchorItem *>(ui->anchorsWidget->itemWidget(item));
    ItemType type = editingAnchorItem->get_type();

    QAction *showFrame = nullptr;
    QAction *changePosition = nullptr;
    QAction *setEndFrame = nullptr;
    QAction *setVideoEnd = nullptr;
    QAction *deleteItem = nullptr;

    QMenu *menu = new QMenu(tr("Edit item"), this);

    if (editingAnchorItem->is_set())
    {
        showFrame = menu->addAction(tr("Show frame"));
    }

    if (type == ItemType::CHANGE || type == ItemType::BEGINNING)
    {
        changePosition = menu->addAction(tr("Change position"));
        changePosition->setToolTip(tr("Change the initial position of this object"));

        if (type == ItemType::CHANGE)
        {
            deleteItem = menu->addAction(tr("Delete item"));
            deleteItem->setToolTip(tr("Delete this item"));
        }
    }
    else if (type == ItemType::END)
    {
        if (editingAnchorItem->is_set())
        {
            setEndFrame = menu->addAction(tr("Change last frame of tracking"));
            setEndFrame->setToolTip(tr("Change the frame where tracking for this object shall stop"));

            setVideoEnd = menu->addAction(tr("Set tracking till the end"));
            setVideoEnd->setToolTip(tr("Set tracking till the end of the video for this object"));
        }
        else // Show different menu label for "setEndFrame", but action is the same
        {
            setEndFrame = menu->addAction(tr("Set last frame of tracking"));
            setEndFrame->setToolTip(tr("Set a frame where tracking for this object shall stop"));
        }
    }

    editingAnchorItem->set_highlight(true); // Keeps the item highlighted even with context menu over it
    //menu->exec(editingAnchorItem->mapToGlobal(QPoint(0,0))); // Show at widget position

    QAction *selectedOption = menu->exec(QCursor::pos()); // Show at cursor position
    editingAnchorItem->set_highlight(false);

    if (!selectedOption)
        return; // No option was selected

    pause(); // Pause video playing (in case it was playing)

    if (selectedOption == showFrame)
    {
        qDebug() << "Show frame";
        show_frame_by_timestamp(editingAnchorItem->get_timestamp());
    }
    else if (selectedOption == changePosition)
    {
        qDebug() << "Change position";

        editingAnchorItem->set_highlight(true);

        // Goes to the event frame so that user does not need to look for it
        show_frame_by_timestamp(editingAnchorItem->get_timestamp());

        set_selection(SelectionState::CHANGE_POSITION);
    }
    else if (selectedOption == setEndFrame)
    {
        set_end_frame();
    }
    else if (selectedOption == setVideoEnd)
    {
        set_video_end();
    }

    else if (selectedOption == deleteItem)
    {
        qDebug() << "Delete item";

        // Ask if they really want to delete this item
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setWindowTitle(tr("Delete trajectory item"));
        msgBox->setText(tr("Do you really want to delete this item?"));
        msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox->setButtonText(QMessageBox::Yes, tr("Yes"));
        msgBox->setButtonText(QMessageBox::No, tr("No"));
        msgBox->setDefaultButton(QMessageBox::Yes);

        if (msgBox->exec() == QMessageBox::Yes)
        { // Deletion confirmed by user
            unsigned int objectID = ui->objectsBox->currentData().toUInt();

            tracker->delete_object_trajectory_section(objectID, editingAnchorItem->get_timestamp());
            set_anchors_tab(objectID);
        }

        projectChanged = true;
    }
    else
    {
        qDebug() << "ERROR: Action from trajectory menu was not recognized";
    }
}

void MainWindow::set_change_beginning()
{
    qDebug() << "Change Beginning position";

    set_anchors_tab(ui->objectsBox->currentData().toUInt());

    QListWidgetItem *beginItem = ui->anchorsWidget->item(0); // The Beginning item
    editingAnchorItem = static_cast<AnchorItem *>(ui->anchorsWidget->itemWidget(beginItem));

    editingAnchorItem->set_highlight(true);

    // Goes to the event frame so that user does not need to look for it
    show_frame_by_timestamp(editingAnchorItem->get_timestamp());

    set_selection(SelectionState::CHANGE_POSITION);
}

void MainWindow::set_end_frame()
{
    qDebug() << "Set end frame";

    set_anchors_tab(ui->objectsBox->currentData().toUInt());
    //int width = ui->anchorsWidget->size().width()-1;
    //qDebug() << width;

    QListWidgetItem *endItem = ui->anchorsWidget->item(ui->anchorsWidget->count()-1); // The End item

    editingAnchorItem = static_cast<AnchorItem *>(ui->anchorsWidget->itemWidget(endItem));

    editingAnchorItem->set_highlight(true);

    if (editingAnchorItem->is_set())
        show_frame_by_timestamp(editingAnchorItem->get_timestamp());

    set_selection(SelectionState::SET_END_FRAME, false);
}

void MainWindow::set_video_end()
{
    qDebug() << "Set video end";

    set_anchors_tab(ui->objectsBox->currentData().toUInt());

    unsigned int objectID = ui->objectsBox->currentData().toUInt();

    tracker->change_object_end_frame(objectID, false);

    projectChanged = true;
}

void MainWindow::set_selection(SelectionState state, bool enable_selecting)
{
    QString text;
    switch (state)
    {
    case SelectionState::NEW_OBJECT:
        text = tr("Select an area for tracking");
        break;

    case SelectionState::CHANGE_POSITION:
        text = tr("Select a new position");
        break;

    case SelectionState::SET_END_FRAME:
        text = tr("Select a frame where you want the object to end");
        break;

    case SelectionState::NEW_SECTION:
        text = tr("Select a new position for this object");
        break;

    default:
        qDebug() << "ERROR: SelectionState not recognized";
        return;
    }

    selectionState = state;
    ui->selectionLabel->setText(text);
    ui->objectsControlWidget->setEnabled(false);
    ui->newObjectButton->setEnabled(false);

    //ui->openVideoButton->setEnabled(false);
    ui->actionOpenVideo->setEnabled(false);
    ui->actionLoadProject->setEnabled(false);
    ui->actionSaveProject->setEnabled(false);

    //ui->createOutputButton->setEnabled(false);
    ui->actionCreateOutput->setEnabled(false);

    ui->selectionWidget->setVisible(true);

    if (enable_selecting)
        ui->videoLabel->set_selection_enabled(true); // Enables selection of area to be tracked

    set_application_menu();
}

// When an object is selected, display its settings
void MainWindow::set_object_settings()
{
    if (!tracker || tracker->get_objects_count() == 0) // No tracking object is added
        return;
    else if (!ask_save_appearance_changes())
        return;

    unsigned int id = ui->objectsBox->currentData().toUInt();
    set_appearance_tab(id);
    set_anchors_tab(id);
}

void MainWindow::set_appearance_tab(unsigned int id)
{
    originalObjectAppearance = tracker->get_object_appearance(id);
    alteredObjectAppearance = originalObjectAppearance;

    settingObjectSettings = true; // Disables slots for appearance change (change_color(), ...)

    ui->shapeBox->setCurrentIndex(ui->shapeBox->findData(originalObjectAppearance.shape));
    //qDebug() << "defocus:" << originalObjectAppearance.defocus;
    ui->defocusButton->setChecked(originalObjectAppearance.defocus);
    ui->colorButton->setChecked(!originalObjectAppearance.defocus);
    ui->defocusSizeBox->setValue(originalObjectAppearance.defocusSize);
    ui->drawInsideBox->setChecked(originalObjectAppearance.drawInside);
    ui->drawBorderBox->setChecked(originalObjectAppearance.drawBorder);
    ui->colorBox->setCurrentIndex(ui->colorBox->findData(originalObjectAppearance.colorID));
    ui->borderColorBox->setCurrentIndex(ui->borderColorBox->findData(originalObjectAppearance.borderColorID));
    ui->borderThicknessBox->setValue(originalObjectAppearance.borderThickness);

    ui->defocusWidget->setEnabled(originalObjectAppearance.defocus);
    ui->colorWidget->setEnabled(!originalObjectAppearance.defocus);

    settingObjectSettings = false;
}

/* if clear==true, trajectory is set from scratch */
void MainWindow::set_trajectory_tab(unsigned int id, bool clear, bool showProgressBar)
{
    if (clear)
        ui->trajectoryWidget->clear(); // Clears to add all items again to empty list

    QListWidgetItem *listItem = nullptr;
    TrajectoryItem *item = nullptr;

    auto trajectory = tracker->get_object_trajectory(id);

    auto iterator = trajectory.begin();
    if (ui->trajectoryWidget->count() > 0)
    { // the list is not empty, find the last one and start adding from the following one

        // Find last item and its timestamp:
        QListWidgetItem *lastItem = ui->trajectoryWidget->item(ui->trajectoryWidget->count()-1); // The Last item
        TrajectoryItem *lastTrajectoryItem = static_cast<TrajectoryItem *>(ui->trajectoryWidget->itemWidget(lastItem));
        int64_t lastTimestamp = lastTrajectoryItem->get_timestamp();

        iterator = trajectory.find(lastTimestamp);

        if (iterator == trajectory.end())
        { // This should never occur; It is here just for a safety reason.
            iterator = trajectory.begin();
            ui->trajectoryWidget->clear();
        }
        else
        {
            iterator++; // Start adding from the next one
        }
    }

    QProgressDialog progressDialog(tr("Setting Trajectory tab"), tr("Cancel"), 0, 0, this);
    if (showProgressBar && iterator != trajectory.end())
    { // This operation might take a while -> show a progress dialog
        progressDialog.setWindowTitle(tr("Trajectory tab"));
        progressDialog.setModal(true);
        progressDialog.show();
    }
    while (iterator != trajectory.end())
    {
        TrajectoryEntry entry = iterator->second;
        listItem = new QListWidgetItem();
        item = new TrajectoryItem(iterator->first, entry.timePosition, tracker->get_total_time(),
                                  entry.frameNumber, tracker->get_frame_count(), entry.position,
                                  this, displayTime);
        ui->trajectoryWidget->addItem(listItem);
        ui->trajectoryWidget->setItemWidget(listItem, item); // Takes ownership of listItem => frees automatically
        listItem->setSizeHint(QSize(listItem->sizeHint().width(), 50));

        if (progressDialog.wasCanceled()) // User cancelled the progress dialog
            return;

        qApp->processEvents(); // Keeps progress bar active

        iterator++;
    }
    progressDialog.cancel();

}

void MainWindow::trajectory_show_frame(QListWidgetItem *item)
{

    TrajectoryItem *selectedItem = static_cast<TrajectoryItem *>(ui->trajectoryWidget->itemWidget(item));
    show_frame_by_timestamp(selectedItem->get_timestamp());
}

void MainWindow::set_anchors_tab(unsigned int id)
{
    assert(tracker != nullptr);


    ui->anchorsWidget->clear(); // Clears to add all items again to empty list

    QListWidgetItem *listItem = nullptr;
    AnchorItem *item = nullptr;

    ItemType type = ItemType::BEGINNING;
    auto trajectorySections = tracker->get_object_trajectory_sections(id);
    for (auto &section: trajectorySections)
    {
        listItem = new QListWidgetItem();
        item = new AnchorItem(type, section.first, section.second.initialTimePosition, tracker->get_total_time(),
                                  section.second.initialFrameNumber, tracker->get_frame_count(), this, true, displayTime);
        ui->anchorsWidget->addItem(listItem);
        ui->anchorsWidget->setItemWidget(listItem, item); // Takes ownership of listItem => frees automatically
        type = ItemType::CHANGE;
    }

    int64_t endTimestamp;
    unsigned long endTimePosition;
    unsigned long endFrameNumber;

    isEndTimestampSet = tracker->get_object_end(id, endTimestamp, endTimePosition, endFrameNumber);
    listItem = new QListWidgetItem();
    item = new AnchorItem(ItemType::END, endTimestamp, endTimePosition, tracker->get_total_time(),
                              endFrameNumber, tracker->get_frame_count(), this, isEndTimestampSet, displayTime);
    ui->anchorsWidget->addItem(listItem);
    ui->anchorsWidget->setItemWidget(listItem, item);

    set_trajectory_tab(id, true, true);

    set_application_menu(); // Because of isEndTimestampSet that might be changed
}

void MainWindow::add_trajectory_change()
{
    set_anchors_tab(ui->objectsBox->currentData().toUInt());
    set_selection(SelectionState::NEW_SECTION);
}

void MainWindow::change_shape()
{
    if (settingObjectSettings)
        return;

    //qDebug() << "Shape changed";
    alteredObjectAppearance.shape = ui->shapeBox->currentData().toUInt();

    change_appearance();
}

void MainWindow::change_defocus()
{
    if (settingObjectSettings)
        return;

    alteredObjectAppearance.defocus = ui->defocusButton->isChecked();

    ui->defocusWidget->setEnabled(alteredObjectAppearance.defocus);
    ui->colorWidget->setEnabled(!alteredObjectAppearance.defocus);

    change_appearance();
}

void MainWindow::change_defocus_size()
{
    if (settingObjectSettings)
        return;

    alteredObjectAppearance.defocusSize = ui->defocusSizeBox->value();

    change_appearance();
}

void MainWindow::change_color()
{
    if (settingObjectSettings)
        return;

    //qDebug() << "Change color";
    unsigned int newColorID = ui->colorBox->currentData().toUInt();
    if (newColorID == 0)
        custom_color();
    else
    {
        alteredObjectAppearance.colorID = newColorID;

        alteredObjectAppearance.color = colorsMap[newColorID].second;

        change_appearance();
    }
}

void MainWindow::change_border_color()
{
    if (settingObjectSettings)
        return;

    //qDebug() << "Change border color";
    unsigned int newColorID = ui->borderColorBox->currentData().toUInt();
    if (newColorID == 0)
        custom_border_color();
    else
    {
        alteredObjectAppearance.borderColorID = newColorID;

        alteredObjectAppearance.borderColor = colorsMap[newColorID].second;

        change_appearance();
    }
}

void MainWindow::change_border_thickness()
{
    if (settingObjectSettings)
        return;

    //qDebug() << "Border thickness changed";
    alteredObjectAppearance.borderThickness = ui->borderThicknessBox->value();

    change_appearance();
}

void MainWindow::change_draw_inside()
{
     if (settingObjectSettings)
        return;

    //qDebug() << "Draw inside changed";
    alteredObjectAppearance.drawInside = ui->drawInsideBox->isChecked();

    change_appearance();
}

void MainWindow::change_draw_border()
{
     if (settingObjectSettings)
        return;

    //qDebug() << "Draw border changed";
    alteredObjectAppearance.drawBorder = ui->drawBorderBox->isChecked();

    change_appearance();
}

void MainWindow::change_appearance()
{

    //qDebug() << "Change appearance";
    if (!appearanceChanged)
    {
        ui->appearanceButtonsWidget->setVisible(true);
        appearanceChanged = true;
    }

    tracker->change_object_appearance(ui->objectsBox->currentData().toUInt(), alteredObjectAppearance);

    if (!tracker->get_current_frame(frame, originalFrame, showOriginalVideo))
        qDebug() << "ERROR: change_appearance(): Cannot show current frame";
    else
        show_frame();
}

bool MainWindow::ask_save_appearance_changes()
{
    if (!appearanceChanged) // No changes to appearance made, everything is all right
        return true;

    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setWindowTitle(tr("Appearance changes"));
    msgBox->setText(tr("Changes to appearance of object") + " \"" + ui->objectsBox->currentText() +
                    "\" "+ tr("were made. Do you wish to save your changes?"));
    msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    msgBox->setButtonText(QMessageBox::Yes, tr("Yes"));
    msgBox->setButtonText(QMessageBox::No, tr("No"));
    msgBox->setButtonText(QMessageBox::Cancel, tr("Cancel"));
    msgBox->setDefaultButton(QMessageBox::Yes);

    int state = msgBox->exec();

    if (state == QMessageBox::Yes)
    {
        confirm_appearance_changes();
        return true;
    }
    else if (state == QMessageBox::No)
    {
        discard_appearance_changes();
        return true;
    }
    else // Cancel button pressed
        return false;
}

void MainWindow::confirm_appearance_changes()
{
    assert(appearanceChanged);
    appearanceChanged = false;
    projectChanged = true;

    originalObjectAppearance = alteredObjectAppearance;
    ui->appearanceButtonsWidget->setVisible(false);

}

void MainWindow::discard_appearance_changes()
{
    assert(appearanceChanged);

    alteredObjectAppearance = originalObjectAppearance;

    change_appearance(); // Is needed to revert changes and show current image

    appearanceChanged = false;

    unsigned int id = ui->objectsBox->currentData().toUInt();
    set_appearance_tab(id); // Sets Appearance tab to original values (state before changes were made)

    ui->appearanceButtonsWidget->setVisible(false);
}

/* Sets items in Appearance tab to behave correctly */
void MainWindow::show_appearance_tab()
{
    settingObjectSettings = true; // Disables signal handlings

    ui->colorBox->clear();
    ui->colorBox->insertItem(0, tr("+ Add new"), 0);
    ui->colorBox->insertSeparator(1);

    ui->borderColorBox->clear();
    ui->borderColorBox->insertItem(0, tr("+ Add new"), 0);
    ui->borderColorBox->insertSeparator(1);

    QPixmap colorPixmap(ui->colorBox->iconSize());
    for (auto const &color: colorsMap)
    {
        ObjectColor const &objColor = color.second.second;
        colorPixmap.fill(QColor(objColor.r, objColor.g, objColor.b));

        QIcon colorIcon(colorPixmap);
        ui->colorBox->addItem(colorIcon, QString::fromStdString(color.second.first), color.first);
        ui->borderColorBox->addItem(colorIcon, QString::fromStdString(color.second.first), color.first);
    }

    ui->shapeBox->clear();
    for (auto const &shape: shapesMap)
    {
       ui->shapeBox->addItem(shape.second, shape.first);
    }

    ui->defocusSizeBox->setRange(2, 10000); // Value 1 would not have any effect; Maximum is much higher than could be needed.
    ui->borderThicknessBox->setRange(0, 255); // OpenCV accepts border thickness in range 0-255

    settingObjectSettings = false;
}

void MainWindow::custom_color()
{
    unsigned int newColorID;
    if (color_picker(newColorID, colorsMap[alteredObjectAppearance.colorID].second))
        ui->colorBox->setCurrentIndex(ui->colorBox->findData(newColorID));
    else // No color selected, set previous color
        ui->colorBox->setCurrentIndex(ui->colorBox->findData(alteredObjectAppearance.colorID));
}

void MainWindow::custom_border_color()
{
    unsigned int newColorID;
    if (color_picker(newColorID, colorsMap[alteredObjectAppearance.borderColorID].second))
        ui->borderColorBox->setCurrentIndex(ui->borderColorBox->findData(newColorID));
    else // No color selected, set previous color
        ui->borderColorBox->setCurrentIndex(ui->borderColorBox->findData(alteredObjectAppearance.borderColorID));
}

bool MainWindow::color_picker(unsigned int &newColorID, ObjectColor const &initialColor)
{
    //static unsigned int num = 1;
    QColorDialog dialog(this);
    QColor selectedColor = dialog.getColor(QColor(initialColor.r, initialColor.g, initialColor.b));
    if (!selectedColor.isValid()) // user hit "Cancel" button
        return false;

    QPixmap colorPixmap(ui->colorBox->iconSize());
    colorPixmap.fill(selectedColor);
    QIcon colorIcon(colorPixmap);

    QString colorName = tr("Custom #") + QString::number(++customColorsCount);

    newColorID = colorsMap.rbegin()->first + 1; // Gets the highest value and increases; careful if changing order e.g. alphabetically
    colorsMap[newColorID] = std::make_pair(colorName.toStdString(), ObjectColor(selectedColor.red(), selectedColor.green(), selectedColor.blue()));

    ui->colorBox->addItem(colorIcon, colorName, newColorID);
    ui->borderColorBox->addItem(colorIcon, colorName, newColorID);

    return true;
}

bool MainWindow::enter_object_name(bool change)
{
    assert(tracker);

    //static unsigned int objectNumber = 1; // Used for default (recommended) text
    unsigned int objectNumber = tracker->get_objects_count() + 1; // Used for default (recommended) text
    //bool ok; // or check "name" for null
    int ok;
    bool firstTime = true;

    QString defaultText;
    QString heading;
    if (change)
    {
        heading = tr("Change object name");

        defaultText = ui->objectsBox->currentText();
    }
    else
    {
        heading = tr("New object");

        defaultText = tr("Object") + " " + QString::number(objectNumber);
        while (ui->objectsBox->findText(defaultText) >= 0) // name already exists, increase default number
            defaultText = tr("Object") + " " + QString::number(++objectNumber);
    }
    while (true)
    { // If user enters a name that already exists, allow him to enter another one
        if (!firstTime)
            defaultText = newObjectName;
        else
            firstTime = false;

       // newObjectName = QInputDialog::getText(this, heading, tr("Object name:"), QLineEdit::Normal, defaultText, &ok);
        QInputDialog dialog(this);
        dialog.setInputMode(QInputDialog::TextInput);
        dialog.setLabelText(tr("Object name:"));
        dialog.setWindowTitle(heading);
        dialog.setTextValue(defaultText);
        dialog.setOkButtonText(tr("Ok"));
        dialog.setCancelButtonText(tr("Cancel"));

        ok = dialog.exec();

        if (ok)
        { // OK button pressed
            newObjectName = dialog.textValue();

            if (newObjectName.isEmpty())
            { // Input string is empty
                QMessageBox *alert = new QMessageBox(this);
                alert->setWindowTitle(tr("Object name"));
                alert->setText(tr("Object name cannot be empty"));
                alert->exec();
                continue;
            }
            else if (ui->objectsBox->findText(newObjectName) >= 0)
            { // Object with this name already exists, show warning and continue with adding

                if (change && newObjectName == ui->objectsBox->currentText())
                { // When changing object name, user left the old name and clicked OK
                    return false;
                }
                else
                {
                    QMessageBox *alert = new QMessageBox(this);
                    alert->setWindowTitle(tr("Object exists"));
                    alert->setText(tr("Object with this name already exists. Enter a new one."));
                    alert->exec();
                    continue;
                }
            }

            return true;

        }
        else // Cancel button pressed
            return false;
    }

}

void MainWindow::add_new_object()
{
    pause();

    if (!ask_save_appearance_changes())
        return;

    if (enter_object_name(false))
    { // User selected name correctly
        set_selection(SelectionState::NEW_OBJECT);
    }
}

void MainWindow::compute_trajectory()
{
    assert(tracker);
    pause();

    // Save timestamp to restore it after the operation is complete
    int64_t currentTimestamp = tracker->get_frame_timestamp();
    unsigned currentObject = ui->objectsBox->currentData().toUInt();

    QProgressDialog progressDialog(computingInfoText, tr("Cancel"), 0, 0, this);
    progressDialog.setWindowTitle(computingInfoTitle);
    progressDialog.setModal(true);
    progressDialog.show();
    QMessageBox *alert = new QMessageBox(this);
    alert->setWindowTitle(tr("Computing trajectory"));
    try
    {
        if (!tracker->track_object(currentObject, &progressDialog))
        {
            qDebug() << "WARNING: Cannot compute trajectory";
            alert->setText(tr("Trajectory was not computed"));
        }
        else
        { //success
            alert->setText(tr("Trajectory was successfully computed"));
        }
    } catch (UserCanceledException)
    {
        if (displayTime)
        {
            TimeConversion::Time timePosition;
            timePosition.raw = tracker->get_time_position();
            TimeConversion::convert_time(timePosition);
            bool showHours = timePosition.hr ? true : false;
            QString timeString;
            TimeConversion::Time2QString(timePosition, timeString, showHours);

            alert->setText(tr("Trajectory computing was cancelled. Last computed frame time position is ")
                           + timeString);
        }
        else
        {
            alert->setText(tr("Trajectory computing was cancelled. Last computed frame's number is ")
                           + QString::number(tracker->get_frame_number()));
        }
    }

    progressDialog.cancel();

    alert->exec();

    set_trajectory_tab(currentObject, false, true);

    show_frame_by_timestamp(currentTimestamp); // restore player position before the computing started
}

void MainWindow::change_name()
{
    if (enter_object_name(true))
    { // User selected name correctly
        tracker->set_object_name(ui->objectsBox->currentData().toUInt(), newObjectName.toStdString());

        show_objects_box(true);
        ui->objectsBox->setCurrentText(newObjectName);
        projectChanged = true;
    }
}

void MainWindow::selection_confirmed()
{
    if (selectionState == SelectionState::NEW_OBJECT)
    {
        Selection selectedPosition = ui->videoLabel->get_selection();
        new_object_confirm(selectedPosition);
    }
    else if (selectionState == SelectionState::CHANGE_POSITION)
    {
        Selection selectedPosition = ui->videoLabel->get_selection();
        if (!change_position_confirm(selectedPosition))
        { // Position cannot take place after the End frame

            QMessageBox *alert = new QMessageBox(this);
            alert->setWindowTitle(tr("Changing position"));
            alert->setText(tr("Beginning frame cannot be after End frame"));
            alert->exec();

            return;
        }
    }
    else if (selectionState == SelectionState::SET_END_FRAME)
    {
        if (!set_end_frame_confirm())
        { // End frame cannot be before Beginning frame

            QMessageBox *alert = new QMessageBox(this);
            alert->setWindowTitle(tr("Setting end frame"));
            alert->setText(tr("End frame cannot be before Beginning frame"));
            alert->exec();

            return;

        }
    }
    else if (selectionState == SelectionState::NEW_SECTION)
    {
        Selection selectedPosition = ui->videoLabel->get_selection();
        if (!new_section_confirm(selectedPosition))
        { // Position cannot take place after the End frame

            QMessageBox *alert = new QMessageBox(this);
            alert->setWindowTitle(tr("New trajectory change"));
            alert->setText(tr("Section cannot begin after End frame"));
            alert->exec();

            return;
        }
    }

    if (!tracker->get_current_frame(frame, originalFrame, showOriginalVideo))
        qDebug() << "ERROR-selection_confirmed(): Cannot show current frame";
    else
        show_frame();

    projectChanged = true;
    selection_end();
}

/* Confirm new trajectory change */
bool MainWindow::new_section_confirm(Selection const &selectedPosition)
{
    if (!tracker->set_object_trajectory_section(ui->objectsBox->currentData().toUInt(), tracker->get_frame_timestamp(),
                                                selectedPosition, tracker->get_time_position(), tracker->get_frame_number()))
        return false;

    set_anchors_tab(ui->objectsBox->currentData().toUInt());
    return true;
}

bool MainWindow::change_position_confirm(Selection const &selectedPosition)
{
    if (!tracker->change_object_trajectory_section(ui->objectsBox->currentData().toUInt(), editingAnchorItem->get_timestamp(),
                                                   tracker->get_frame_timestamp(), selectedPosition, tracker->get_time_position(),
                                                   tracker->get_frame_number()))
        return false;

    set_anchors_tab(ui->objectsBox->currentData().toUInt());
    return true;
}

bool MainWindow::set_end_frame_confirm()
{
    if (!tracker->change_object_end_frame(ui->objectsBox->currentData().toUInt(), true, tracker->get_frame_timestamp(),
                                          tracker->get_time_position(), tracker->get_frame_number()))
       return false;

    set_anchors_tab(ui->objectsBox->currentData().toUInt());
    return true;
}

void MainWindow::new_object_confirm(Selection const &selectedPosition)
{
    assert(selectionState == SelectionState::NEW_OBJECT);

    unsigned defaultColor = Colors::BLACK;
    unsigned defaultBorderColor = Colors::RED;

    ObjectColor const &color = colorsMap[defaultColor].second;
    ObjectColor const &borderColor = colorsMap[defaultBorderColor].second;

    Characteristics defaultAppearance(ObjectShape::RECTANGLE, true, 20, true, color,
                                      Colors::BLACK, true, borderColor, Colors::RED, 3);


    int id;
    if ((id = tracker->add_object(defaultAppearance, newObjectName.toStdString(), selectedPosition, tracker->get_frame_timestamp(),
                                  tracker->get_time_position(), tracker->get_frame_number(), false, 0, 0, 0)) < 0)
    {
        qDebug() << "ERROR-new_object_confirm(): Object wasn't added correctly";
        return;
    }

    //ui->videoLabel->set_image(frame, ui->videoFrame->width(), ui->videoFrame->height());

    show_objects_box(true);
    if (ui->objectsBox->currentText() == newObjectName)
    { // the object is already active. thus, tabs need to be updated manually
        set_object_settings();
    }
    else
    { // Changing current text updates all tabs
        ui->objectsBox->setCurrentText(newObjectName); // this changes values in all 3 tabs //test
    }
}

void MainWindow::show_objects_box(bool noTabsUpdate)
{
    // First disconnect this signal so alterig ui->objectsBox does not fire it every time
    // Connect it again at the end of this function
    if (noTabsUpdate)
        QObject::disconnect(ui->objectsBox, SIGNAL(currentIndexChanged(int)), this, SLOT(set_object_settings()));

    ui->objectsBox->clear();

    std::vector<std::string> objects;

    if (!tracker || (objects=tracker->get_all_objects_names()).empty())
    { // No objects exist, disable Objects menu
        ui->objectsBox->addItem(tr("No objects added"));
        ui->objectsBox->setEnabled(false);
        ui->tabWidget->setEnabled(false);
    }
    else
    {
        unsigned int i = 0;
        for (std::string const &object: objects)
        {
            ui->objectsBox->setEnabled(true);
            ui->tabWidget->setEnabled(true);

            ui->objectsBox->addItem(QString::fromStdString(object), i);
            i++;
        }
    }

    set_application_menu();

    // Connect the signal that was disconnected at the beginning of this function
    if (noTabsUpdate)
        QObject::connect(ui->objectsBox, SIGNAL(currentIndexChanged(int)), this, SLOT(set_object_settings()));
}

void MainWindow::selection_end()
{
    assert(selectionState != SelectionState::NO_SELECTION);

    if (selectionState == SelectionState::CHANGE_POSITION || selectionState == SelectionState::SET_END_FRAME)
    {
        editingAnchorItem->set_highlight(false);
        editingAnchorItem = nullptr;
    }

    ui->objectsControlWidget->setEnabled(true);
    ui->newObjectButton->setEnabled(true);
    //ui->openVideoButton->setEnabled(true);
    ui->actionOpenVideo->setEnabled(true);
    ui->actionLoadProject->setEnabled(true);
    ui->actionSaveProject->setEnabled(true);

    //ui->createOutputButton->setEnabled(true);
    ui->actionCreateOutput->setEnabled(true);

    ui->selectionWidget->setVisible(false);
    ui->videoLabel->set_selection_enabled(false); // Disables selection of area to be tracked

    selectionState = SelectionState::NO_SELECTION;

    set_application_menu();
}

void MainWindow::delete_object()
{
    // Ask if they really want to delete this object
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setWindowTitle(tr("Delete object"));
    msgBox->setText(tr("Do you really want to delete object") + " \"" + ui->objectsBox->currentText() + "\"?");
    msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox->setButtonText(QMessageBox::Yes, tr("Yes"));
    msgBox->setButtonText(QMessageBox::No, tr("No"));
    msgBox->setDefaultButton(QMessageBox::Yes);

    if (msgBox->exec() == QMessageBox::Yes)
    { // Deletion confirmed by user

        tracker->delete_object(ui->objectsBox->currentData().toUInt());
        appearanceChanged = false; // Object is deleted -> no object's appearance changes

        show_objects_box(); // Changes were made, display current objectsBox

        // Display current frame (without deleted object)
        if (!tracker->get_current_frame(frame, originalFrame, showOriginalVideo))
            qDebug() << "ERROR-delete_object: Cannot show current frame";
        else
            show_frame();

        projectChanged = true;
    }

}

void MainWindow::display_time()
{
    displayTime = true;
    settings->setValue("showTime", true);

    ui->actionTime->setChecked(true);
    ui->actionFrameNumbers->setChecked(false);


    if (tracker)
    {   // Video is loaded, display its time
        for (int i = 0; i < ui->anchorsWidget->count(); i++)
        {
            QListWidgetItem* item = ui->anchorsWidget->item(i);
            AnchorItem * anchorItem = static_cast<AnchorItem *>(ui->anchorsWidget->itemWidget(item));
            anchorItem->display_time();
        }

        for (int i = 0; i < ui->trajectoryWidget->count(); i++)
        {
            QListWidgetItem* item = ui->trajectoryWidget->item(i);
            TrajectoryItem* trajectoryItem = static_cast<TrajectoryItem *>(ui->trajectoryWidget->itemWidget(item));
            trajectoryItem->display_time();
        }

        ui->timeLabel->display_time(tracker->get_time_position());
    }
    else // Shows default text when no video is loaded
        ui->timeLabel->setText("--:--");

   }

void MainWindow::display_frame_numbers()
{
    displayTime = false;
    settings->setValue("showTime", false);

    ui->actionFrameNumbers->setChecked(true);
    ui->actionTime->setChecked(false);

    if (tracker)
    {   // Video is loaded, display its frame numbers
        for (int i = 0; i < ui->anchorsWidget->count(); i++)
        {
            QListWidgetItem* item = ui->anchorsWidget->item(i);

            AnchorItem * anchorItem = static_cast<AnchorItem *>(ui->anchorsWidget->itemWidget(item));
            anchorItem->display_frame_num();
        }

        for (int i = 0; i < ui->trajectoryWidget->count(); i++)
        {
            QListWidgetItem* item = ui->trajectoryWidget->item(i);

            TrajectoryItem* trajectoryItem = static_cast<TrajectoryItem *>(ui->trajectoryWidget->itemWidget(item));
            trajectoryItem->display_frame_num();
        }

        ui->timeLabel->display_frame_num(tracker->get_frame_number());
    }
    else // Shows default text when no video is loaded
        ui->timeLabel->setText("--/--");

}

void MainWindow::show_original_video()
{
    showOriginalVideo = !showOriginalVideo;

    settings->setValue("showOriginalVideo", showOriginalVideo);

    ui->actionShowOriginalVideo->setChecked(showOriginalVideo);
    ui->originalVideoFrame->setVisible(showOriginalVideo);

    if (showOriginalVideo && tracker)
    {   // Video is loaded, current frame needs to be loaded since originalFrame does not exist yet

        if (!tracker->get_current_frame(frame, originalFrame, showOriginalVideo))
            qDebug() << "ERROR: show_original_video(): Cannot show current frame";
        else
            show_frame();
    }

}

void MainWindow::set_czech_language()
{
    ui->actionPlay->setEnabled(false);
    qDebug() << "Czech language set";

    // Only one language item must be checked
    ui->actionCzech->setChecked(true);
    ui->actionEnglish->setChecked(false);
    settings->setValue("language", ":/language/video_anonymizer_cs");

    QMessageBox *alert = new QMessageBox(this);
    alert->setWindowTitle("Čeština"); // No need for translation (tr()) - display message in selected language
    alert->setText("Změna se projeví až při novém spuštění aplikace");
    alert->exec();

}

void MainWindow::set_english_language()
{
    qDebug() << "English set";

    // Only one language item must be checked
    ui->actionEnglish->setChecked(true);
    ui->actionCzech->setChecked(false);
    settings->setValue("language", "");

    QMessageBox *alert = new QMessageBox(this);
    alert->setWindowTitle("English"); // No need for translation (tr()) - display message in selected language
    alert->setText("The change will take effect after restarting the application");
    alert->exec();

}

bool MainWindow::ask_save_project()
{
    if (!ask_save_appearance_changes()) // Before saving the project, changed appearance of tracked objects must be saved or discarded
            return false;

    if (!tracker || !projectChanged) // Project is empty, cannot be saved
        return true;

    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setWindowTitle(tr("Save project"));
    msgBox->setText(tr("Do you wish to save the current project?"));
    msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);

    msgBox->setStandardButtons(msgBox->standardButtons() | QMessageBox::Cancel);
    msgBox->setButtonText(QMessageBox::Cancel, tr("Cancel"));

    msgBox->setButtonText(QMessageBox::Yes, tr("Yes"));
    msgBox->setButtonText(QMessageBox::No, tr("No"));
    msgBox->setDefaultButton(QMessageBox::Yes);

    int state = msgBox->exec();

    if (state == QMessageBox::Yes)
        return save_project();
    else if (state == QMessageBox::No)
        return true;
    else // Cancel button pressed
        return false;
}

bool MainWindow::save_project()
{
    assert(tracker);

    if (!ask_save_appearance_changes())
        return false;

    QString jsonFilter = "JSON (*.json)";
    QString xmlFilter = "XML (*.xml)";
    QString filters = jsonFilter + ";;" + xmlFilter;
    QString selectedFilter;

    // projectFormat is saved in settings so the last choice is remembered even after restarting the application
    if (settings->contains("projectFormat"))
    { // Is projectFormat variable set in settings?
        selectedFilter = settings->value("projectFormat").toString();
    }
    else // default
        selectedFilter = jsonFilter;

    QString defaultPath = projectFileName;  // If project was loaded, sets its destination, otherwise is empty

    if (defaultPath.isEmpty() && settings->contains("lastPath"))
    { // begin with the directory where the last video was opened
        defaultPath = settings->value("lastPath").toString();
        qDebug() << "last path:" << defaultPath;
    }

    QString filename = QFileDialog::getSaveFileName(this, tr("Select project file name"),
                                                    defaultPath,
                                                    filters, &selectedFilter);
    if (filename.isEmpty())
        return false; // No filename selected

    settings->setValue("projectFormat", selectedFilter);

    { // Serialize with CEREAL

        if (selectedFilter == xmlFilter)
        { // XML

            if (!filename.endsWith(".xml"))
                filename += ".xml"; // If it doesn't have the right extension, append it
            std::ofstream os(filename.toStdString());

            cereal::XMLOutputArchive oarchive(os);

            oarchive(CEREAL_NVP(inputFileName), CEREAL_NVP(tracker), CEREAL_NVP(customColorsCount), cereal::make_nvp("colors", colorsMap));
            //oarchive(inputFileName, tracker, colorsMap);
        }
        else
        { // JSON
            if (!filename.endsWith(".json"))
                filename += ".json"; // If it doesn't have the right extension, append it

            std::ofstream os(filename.toStdString());

            cereal::JSONOutputArchive oarchive(os);

            //oarchive(inputFileName, tracker, colorsMap);
            oarchive(CEREAL_NVP(inputFileName), CEREAL_NVP(tracker), CEREAL_NVP(customColorsCount), cereal::make_nvp("colors", colorsMap));
        }

    }

    projectFileName = filename;

    int projectNameBeginning = projectFileName.lastIndexOf("/");
    if (projectNameBeginning < 0)
        projectNameBeginning = 0;
    else
        projectNameBeginning++;
    projectName = projectFileName.mid(projectNameBeginning);
    set_application_menu(); // It shows the window's title with the project's name

    projectChanged = false;

    return true;
}

void MainWindow::load_project()
{
    if (tracker)
    { // Project exists, ask to save/discard
        if (!ask_save_project())
            return; // User canceled the dialog, do not open a new video

        initial_application_settings(); // Restores default application settings

    }

    QString jsonFilter = "JSON (*json)";
    QString xmlFilter = "XML (*.xml)";
    QString filters = jsonFilter + ";;" + xmlFilter;
    QString selectedFilter;

    // projectFormat is saved in settings so the last choice is remembered even after restarting the application
    if (settings->contains("projectFormat"))
    { // Is projectFormat variable set in settings?
        selectedFilter = settings->value("projectFormat").toString();
    }
    else
        selectedFilter = "JSON (*json)";


    QString defaultDir = "";

    if (settings->contains("lastPath"))
    { // begin with the directory where the last video was opened
        defaultDir = settings->value("lastPath").toString();
        qDebug() << "last path:" << defaultDir;
    }

    projectFileName = QFileDialog::getOpenFileName(this, tr("Load project"),
                                                    defaultDir,
                                                    filters, &selectedFilter);
    if (projectFileName.isEmpty())
    {
        qDebug() << "projectFileName is empty";
        return; // No filename selected
    }


    settings->setValue("projectFormat", selectedFilter); // Remember file format choice also after restarting the application

    colorsMap.clear();
    try
    {
        { // Deserialize with CEREAL

        std::ifstream os(projectFileName.toStdString());

        if (selectedFilter == xmlFilter)
        { // XML
            cereal::XMLInputArchive iarchive(os);
            iarchive(CEREAL_NVP(inputFileName), CEREAL_NVP(tracker), CEREAL_NVP(customColorsCount), cereal::make_nvp("colors", colorsMap));
            //iarchive(inputFileName, tracker, colorsMap);
        }
        else
        { // JSON
            cereal::JSONInputArchive iarchive(os);
            iarchive(CEREAL_NVP(inputFileName), CEREAL_NVP(tracker), CEREAL_NVP(customColorsCount), cereal::make_nvp("colors", colorsMap));
            //iarchive(inputFileName, tracker, colorsMap);
        }

        }

    } catch (...) {
        QMessageBox *alert = new QMessageBox(this);
        alert->setWindowTitle(tr("Project opening"));
        alert->setText(tr("Project could not be loaded"));
        alert->exec();
        return;
    }

    while (true)
    { // loading video - until correct video is loaded or user pressed "Cancel"
        QProgressDialog progressDialog(tr("Opening video"), tr("Cancel"), 0, 0, this);
        progressDialog.setWindowTitle(tr("Opening"));
        progressDialog.setModal(true);
        progressDialog.show();

        try
        {

            tracker->load_video(inputFileName, &progressDialog); // inputFileName was loaded with the loaded project
        } catch (OpenException) {

            qDebug() << "Video cannot be opened.";

            QMessageBox *alert = new QMessageBox(this);
            alert->setWindowTitle(tr("Opening video"));
            alert->setText(tr("Video from the project was not found or cannot be opened. Please set its path."));
            alert->exec();

            if (!open_video_dialog()) // Shows dialog to select a file
            { // Cancel button pressed
                tracker = nullptr;
                return;
            }
            else // Video file selected, try opening again
                continue;

        } catch (UserCanceledOpeningException) {

            QMessageBox *alert = new QMessageBox(this);
            alert->setWindowTitle(tr("Opening video"));
            alert->setText(tr("Video opening was canceled."));
            alert->exec();

            return;
        }

        break;
    }

    tracker->erase_object_trajectories_to_comply();

    int projectNameBeginning = projectFileName.lastIndexOf("/");

    if (projectNameBeginning < 0)
        projectNameBeginning = 0;
    else
        projectNameBeginning++;
    projectName = projectFileName.mid(projectNameBeginning);
    // set_application_menu() is needed to show the window's title
    // with the project's name. However, the open_video_successful()
    // calls the function.

    open_video_successful();
    show_objects_box();
}

void MainWindow::show_help()
{
    qDebug() << "showing help";
    QTabWidget* tWidget = new QTabWidget;
    tWidget->setMaximumWidth(200);
    tWidget->addTab((QWidget*)helpEngine->contentWidget(), tr("Contents"));
    tWidget->addTab((QWidget*)helpEngine->indexWidget(), tr("Index"));
    HelpBrowser *textViewer = new HelpBrowser(helpEngine);
    //textViewer->setSource( QUrl("qthelp://walletfox.qt.helpexample/doc/index.html"));
    textViewer->setSource(QUrl("qthelp://fit.cz.videoanonymizer/doc/" + tr("en_doc.html")));

    connect((QWidget*)helpEngine->contentWidget(), SIGNAL(linkActivated(QUrl)), textViewer, SLOT(setSource(QUrl)));

    connect((QWidget*)helpEngine->indexWidget(), SIGNAL(linkActivated(QUrl, QString)), textViewer, SLOT(setSource(QUrl)));

    QSplitter *horizSplitter = new QSplitter(Qt::Horizontal);
    horizSplitter->insertWidget(0, tWidget);
    horizSplitter->insertWidget(1, textViewer);
    horizSplitter->hide();

    QDockWidget *helpWindow = new QDockWidget(tr("Help"), this);
    helpWindow->setWidget(horizSplitter);
  //  helpWindow->hide();
    addDockWidget(Qt::BottomDockWidgetArea, helpWindow);
    qDebug() << "help showed";

}

void MainWindow::show_about()
{
    QMessageBox::about(this, tr("About Video Anonymizer"),
                       tr("This application is used for anonymizing objects in a video.\n") +
                       tr("Created by Martin Borek.\n") + "2015");

}

void MainWindow::initial_application_settings()
{
    if (tracker)
    {
        //delete tracker;
        tracker = nullptr; //is deleted automatically as tracker is a smart pointer
    }

    if (timer)
        timer->stop();

    isPlaying = false;
    selectionState = SelectionState::NO_SELECTION;
    editingAnchorItem = nullptr;
    appearanceChanged = false;
    projectChanged = false;
    settingObjectSettings = false; // Disables slots for appearance change (change_color(), ...) when setting new object
    isEndTimestampSet = false;

    projectFileName = "";
    projectName = "";

    timerInterval = 0; // Original interval based on fps
    timerSpeed = 0; // power used with constant VIDEOTRACKING_TIMER_CONSTANT to play video faster/slower

    customColorsCount = 0;

    ui->selectionWidget->setVisible(false);
    ui->appearanceButtonsWidget->setVisible(false);
    ui->originalVideoTextLabel->setVisible(false);

    ui->playerControlsWidget->setEnabled(false);
    ui->positionSlider->setEnabled(false);
    ui->objectsControlWidget->setEnabled(false);

    //ui->createOutputButton->setEnabled(false);
    ui->actionCreateOutput->setEnabled(false);


    // Move position slider to the beginning
    ui->positionSlider->setValue(ui->positionSlider->minimum());

    set_application_menu();

    set_form_values();
    //show_appearance_tab(); // It is called from open_video_successful()

    show_objects_box();

    // Display default time/frame numbers label
    if (displayTime)
        display_time();
    else
        display_frame_numbers();

    // Make the videoLabel empty
    ui->videoLabel->set_selection_enabled(false);
    ui->videoLabel->clear();
    ui->originalVideoLabel->set_selection_enabled(false);
    ui->originalVideoLabel->clear();
}
/**
void MainWindow::exit_application()
{
    if (!ask_save_project()) // User canceled the dialog telling him to Save or Discard the current project
        return;
}
*/

void MainWindow::set_application_menu()
{
    // Set window title:
    if (projectName.isEmpty())
    {
        if (tracker)
            setWindowTitle(tr("Unsaved Project") + " | " + applicationName);
        else
            setWindowTitle(applicationName);
    }
    else
        setWindowTitle(projectName + " | " + applicationName);

    ui->actionPlay->setEnabled(ui->playButton->isEnabled());
    ui->actionStop->setEnabled(ui->stopButton->isEnabled());
    ui->actionStepBack->setEnabled(ui->stepBackButton->isEnabled());
    ui->actionStepForward->setEnabled(ui->stepForwardButton->isEnabled());

    ui->actionSlower->setEnabled(ui->slowerButton->isEnabled());
    ui->actionFaster->setEnabled(ui->fasterButton->isEnabled());

    if (tracker)
        ui->actionSaveProject->setEnabled(true);
    else
        ui->actionSaveProject->setEnabled(false);

    ui->actionNewObject->setEnabled(ui->newObjectButton->isEnabled());

    if (ui->generalTab->isEnabled())
    {
        ui->actionChangeName->setEnabled(true);
        ui->actionRemoveObject->setEnabled(true);
    }
    else
    {
        ui->actionChangeName->setEnabled(false);
        ui->actionRemoveObject->setEnabled(false);
    }

    ui->menuTrajectory->setEnabled(ui->anchorsTab->isEnabled());
    ui->actionComputeTrajectory->setEnabled(ui->computeTrajectoryButton->isEnabled());
    ui->actionVideoEnd->setChecked(!isEndTimestampSet);

    ui->actionShowOriginalVideo->setChecked(showOriginalVideo);

}

/**
 * Connects signals (from GUI elements and others) with slots of this object.
 * @brief MainWindow::connect_signals
 */
void MainWindow::connect_signals()
{
    // Signals are automatically disconnected with QObject destructor

    // PLAYER CONTROL SECTION
    QObject::connect(ui->playButton, SIGNAL(clicked()), this, SLOT(play()));
    QObject::connect(ui->actionPlay, SIGNAL(triggered()), this, SLOT(play()));

    QObject::connect(ui->stopButton, SIGNAL(clicked()), this, SLOT(stop()));
    QObject::connect(ui->actionStop, SIGNAL(triggered()), this, SLOT(stop()));

    QObject::connect(ui->stepForwardButton, SIGNAL(clicked()), this, SLOT(step_forward()));
    QObject::connect(ui->actionStepForward, SIGNAL(triggered()), this, SLOT(step_forward()));

    QObject::connect(ui->stepBackButton, SIGNAL(clicked()), this, SLOT(step_back()));
    QObject::connect(ui->actionStepBack, SIGNAL(triggered()), this, SLOT(step_back()));

    //QObject::connect(ui->fasterButton, SIGNAL(clicked()), this, SLOT(faster()));
    //QObject::connect(ui->slowerButton, SIGNAL(clicked()), this, SLOT(slower()));

    QObject::connect(ui->fasterButton, SIGNAL(clicked()), this, SLOT(faster()));
    QObject::connect(ui->actionFaster, SIGNAL(triggered()), this, SLOT(faster()));

    QObject::connect(ui->slowerButton, SIGNAL(clicked()), this, SLOT(slower()));
    QObject::connect(ui->actionSlower, SIGNAL(triggered()), this, SLOT(slower()));

    QObject::connect(ui->actionOriginalSpeed, SIGNAL(triggered()), this, SLOT(original_speed()));

    //QObject::connect(ui->positionSlider, SIGNAL(sliderMoved(int)), this, SLOT(show_frame_by_time(int)));
    QObject::connect(ui->positionSlider, SIGNAL(clicked(ulong)), this, SLOT(show_frame_by_number(ulong)));
    QObject::connect(ui->positionSlider, SIGNAL(sliderPressed()), this, SLOT(slider_pressed()));
    QObject::connect(ui->positionSlider, SIGNAL(sliderReleased()), this, SLOT(slider_released()));
    //QObject::connect(ui->positionSlider, SIGNAL(), this, SLOT(show_frame_by_number(int)));


    // FILE
    //QObject::connect(ui->openVideoButton, SIGNAL(clicked()), this, SLOT(open_video()));
    QObject::connect(ui->actionOpenVideo, SIGNAL(triggered()), this, SLOT(open_video()));

    //QObject::connect(ui->createOutputButton, SIGNAL(clicked()), this, SLOT(create_output()));
    QObject::connect(ui->actionCreateOutput, SIGNAL(triggered()), this, SLOT(create_output()));

    QObject::connect(ui->actionSaveProject, SIGNAL(triggered()), this, SLOT(save_project()));
    QObject::connect(ui->actionLoadProject, SIGNAL(triggered()), this, SLOT(load_project()));
    QObject::connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));

    // OBJECTS
    QObject::connect(ui->objectsBox, SIGNAL(currentIndexChanged(int)), this, SLOT(set_object_settings()));

    QObject::connect(ui->newObjectButton, SIGNAL(clicked()), this, SLOT(add_new_object()));
    QObject::connect(ui->actionNewObject, SIGNAL(triggered()), this, SLOT(add_new_object()));


    // OBJECT APPEARANCE TAB
    QObject::connect(ui->shapeBox, SIGNAL(currentIndexChanged(int)), this, SLOT(change_shape()));

    //handling defocusButton handles also colorButton since toggling colorButton switches off defocusButton
    QObject::connect(ui->defocusButton, SIGNAL(toggled(bool)), this, SLOT(change_defocus()));
    QObject::connect(ui->defocusSizeBox, SIGNAL(valueChanged(int)), this, SLOT(change_defocus_size()));
    QObject::connect(ui->drawInsideBox, SIGNAL(stateChanged(int)), this, SLOT(change_draw_inside()));
    QObject::connect(ui->drawBorderBox, SIGNAL(stateChanged(int)), this, SLOT(change_draw_border()));
    QObject::connect(ui->colorBox, SIGNAL(currentIndexChanged(int)), this, SLOT(change_color()));
    QObject::connect(ui->borderColorBox, SIGNAL(currentIndexChanged(int)), this, SLOT(change_border_color()));
    QObject::connect(ui->borderThicknessBox, SIGNAL(valueChanged(int)), this, SLOT(change_border_thickness()));

    QObject::connect(ui->saveAppearanceChangesButton, SIGNAL(clicked()), this, SLOT(confirm_appearance_changes()));
    QObject::connect(ui->discardAppearanceChangesButton, SIGNAL(clicked()), this, SLOT(discard_appearance_changes()));


    // OBJECT APPEARANCE - CUSTOM COLORS
    //QObject::connect(ui->colorBox, SIGNAL(activated(int)), this, SLOT(custom_color(int)));
    //QObject::connect(ui->borderColorBox, SIGNAL(activated(int)), this, SLOT(custom_border_color(int)));


    // OBJECT ANCHORS TAB
    QObject::connect(ui->anchorsWidget, SIGNAL(itemPressed(QListWidgetItem*)), this, SLOT(show_anchors_menu(QListWidgetItem*)));

    QObject::connect(ui->addTrajectoryChangeButton, SIGNAL(clicked()), this, SLOT(add_trajectory_change()));
    QObject::connect(ui->actionChangeTrajectory, SIGNAL(triggered()), this, SLOT(add_trajectory_change()));

    QObject::connect(ui->actionVideoEnd, SIGNAL(triggered()), this, SLOT(set_video_end()));
    QObject::connect(ui->actionSetEndFrame, SIGNAL(triggered()), this, SLOT(set_end_frame()));
    QObject::connect(ui->actionChangeBeginning, SIGNAL(triggered()), this, SLOT(set_change_beginning()));


    // OBJECT TRAJECTORY TAB
    QObject::connect(ui->trajectoryWidget, SIGNAL(itemPressed(QListWidgetItem*)), this, SLOT(trajectory_show_frame(QListWidgetItem*)));

    QObject::connect(ui->computeTrajectoryButton, SIGNAL(clicked()), this, SLOT(compute_trajectory()));
    QObject::connect(ui->actionComputeTrajectory, SIGNAL(triggered()), this, SLOT(compute_trajectory()));

    // OBJECT GENERAL TAB
    QObject::connect(ui->changeNameButton, SIGNAL(clicked()), this, SLOT(change_name()));
    QObject::connect(ui->actionChangeName, SIGNAL(triggered()), this, SLOT(change_name()));

    QObject::connect(ui->removeObjectButton, SIGNAL(clicked()), this, SLOT(delete_object()));
    QObject::connect(ui->actionRemoveObject, SIGNAL(triggered()), this, SLOT(delete_object()));


    // SELECTION
    QObject::connect(ui->confirmSelectionButton, SIGNAL(clicked()), this, SLOT(selection_confirmed()));
    QObject::connect(ui->cancelSelectionButton, SIGNAL(clicked()), this, SLOT(selection_end()));


    // LANGUAGE MENU
    QObject::connect(ui->actionEnglish, SIGNAL(triggered()), this, SLOT(set_english_language()));
    QObject::connect(ui->actionCzech, SIGNAL(triggered()), this, SLOT(set_czech_language()));

    // SETTINGS MENU
    QObject::connect(ui->actionTime, SIGNAL(triggered()), this, SLOT(display_time()));
    QObject::connect(ui->actionFrameNumbers, SIGNAL(triggered()), this, SLOT(display_frame_numbers()));

    QObject::connect(ui->actionShowOriginalVideo, SIGNAL(triggered()), this, SLOT(show_original_video()));

    // OTHERS
    QObject::connect(ui->videoFrame, SIGNAL(resized()), this, SLOT(reload_video_label()));
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(show_next_frame()));

    QObject::connect(ui->actionHelp, SIGNAL(triggered()), this, SLOT(show_help()));
    QObject::connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(show_about()));
    //QObject::connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(exit_application()));
 }
