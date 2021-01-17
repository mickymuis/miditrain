/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

#pragma once

#include <QThread>
#include <QMap>
#include <QMultiMap>
#include <QElapsedTimer>
#include "miditrain.h"
#include "eventqueue.h"

#define PRECISION 2 // msec
#define MAX_IDLE 500

class Composition;
class QMidiOut;
class QMidiEvent;

class PlayThread : public QThread {
    Q_OBJECT
public:
//    typedef QMultiMap<double /*time*/,QMidiEvent*> EventQueueT;
//    typedef QMap<int /*track*/,int /*section*/> SectionQueueT;

    PlayThread( QObject *parent =0 );
    ~PlayThread();

    void setComposition( const Composition* );
    const Composition* composition() const { return _comp; }

    //void setPlayHead( const PlayHead& );
    //PlayHead playHead() const { return _playhead; }

    EventQueue& queue() { return _queue; }

    void setMidiOut( QMidiOut* );
    QMidiOut* midiOut() const { return _midiout; }

    void setTimer( const QElapsedTimer& t );
//    void setStartTime( qint64 origin, qint64 now );

    void run() override;

    void stop() { requestInterruption(); }

//signals:
//    void positionAdvanced( PlayHead );

private:
    void processEvent( const EventQueue::Event* );

    const Composition* _comp;
    //PlayHead _playhead;
    EventQueue _queue;
    QElapsedTimer _timer;
    QMidiOut* _midiout;
    bool _stop;
    TimeVarT _previous;
    //EventQueueT _eventq;
    //SectionQueueT _sectionq;

};
