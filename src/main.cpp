/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

#include <QApplication>
#include <QSurfaceFormat>
#include <cstdio>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    //Q_INIT_RESOURCE(miditrain);

    QApplication app(argc, argv);

    MainWindow window;
    window.show();

     for( int i =1; i < argc; i++ ) {
        // TODO: make commandline flags
        window.openJsonFile( QString( argv[i] ) );
    }

    return app.exec();
}
