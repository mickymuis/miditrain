/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

#pragma once

#include "miditrain.h"
#include "playthread.h"

#include <QMainWindow>
#include <QTimer>
#include <QElapsedTimer>

#define DISPLAY_PRECISION 10

class ScoreWidget;
class Composition;
class EventQueue;
//class PlayHead;
//class PlayThread;
class QMidiOut;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

    bool openJsonFile( const QString& path );

    void setComposition( Composition* );
    Composition* composition() const { return _composition; }

public slots:
    void togglePlayback();
    void start();
    void stop();
    //void updatePosition( PlayHead );

private slots:
    void tick();

private:
    ScoreWidget* _scoreWidget;
    //PlayHead *_playhead;
    EventQueue _queue;
    Composition* _composition;
    PlayThread* _thread;
    QMidiOut *_midiout;
    bool _playing;
    bool _restart;
    qint64 _stoptime;
    QTimer* _timer;
    TimeVarT _previous;
    QElapsedTimer _time;
};

