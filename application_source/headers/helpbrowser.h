/**
 * @file helpbrowser.h
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#ifndef HELPBROWSER_H
#define HELPBROWSER_H

#include <QTextBrowser>
#include <QHelpEngine>
#include <QWidget>

class HelpBrowser : public QTextBrowser
{
public:
    /**
     * Constructor
     * @param helpEngine Help engine with loaded help files
     * @param parent Parent Widget
     */
    HelpBrowser(QHelpEngine *helpEngine, QWidget *parent = 0);

    /**
     * Loading a resource.
     * @param type Resourse type
     * @param name Resource name
     * @return Found data
     */
    QVariant loadResource (int type, const QUrl& name);
private:
    QHelpEngine* helpEngine;

};

#endif // HELPBROWSER_H
