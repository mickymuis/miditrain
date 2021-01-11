/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

#pragma once

#include <QVector>

class Composition;
class QMidiEvent;
class Track;
class Trigger;

class EventQueue {
public:
    struct Event {
        qint64 timestamp;
        const QMidiEvent* midi;
        const Trigger* trigger;

    };

    typedef QVector<Event> EventVectorT;

    struct TrackQueue {
        qint64 length;          // Length in timestamp (msec)
        const Track* track;     // Corresponsing Track object
        EventVectorT events;    // Vector of (midi)events
        int cursor, lap;        // Index in the event vector, n-th repeat cycle
    };

    typedef QVector<TrackQueue> TrackQueueVectorT;
    
    EventQueue();
    ~EventQueue();

    void initialize( const Composition* );

    void restart( qint64 origin, qint64 now );

    void advance( qint64 now );

    Event* takeFront( qint64 now =-1 );

    qint64 origin() const { return _origin; }
    qint64 now() const { return _now; }
    qint64 minTimeUntilNextEvent( qint64 max ) const;
    inline qint64 minTimeUntilNextEvent( qint64 max, qint64 now ) { _now=now; return minTimeUntilNextEvent( max ); }
    inline qint64 elapsedTime() const { return _now - _origin; }

private:
    void addTrack( const Track* t, const Composition* );

    TrackQueueVectorT _tracks;
    qint64 _origin;
    qint64 _now;

};
