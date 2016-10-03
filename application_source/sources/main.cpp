/**
 * @file main.cpp
 * @author Martin Borek (mborekcz@gmail.com)
 * @date May, 2015
 */

#include "mainwindow.h"
#include <QApplication>
//#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    MainWindow window;
    window.show();

    return application.exec();
}
