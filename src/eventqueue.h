/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

#pragma once

#include <QVector>
#include "composition.h"

//class Composition;
class QMidiEvent;
//class Track;
//class Trigger;
//class Trigger::Event;

class EventQueue {
public:
    struct Event {
        qint64 timestamp;
        const Trigger::Event* event;
        const Trigger* trigger;
        const Track* track;

    };

    typedef QVector<Event> EventVectorT;

    struct TrackQueue {
        qint64 length, offset;  // Length in timestamp (msec)
        const Track* track;     // Corresponding Track object
        EventVectorT events;    // Vector of queued (midi)events
        int cursor, lap;        // Index in the event vector, n-th repeat cycle
        bool start, running;    // Track should start when playback is started, track is currently running
        qint64 runningTime, startTime;  // Time running so far, timestamp of start point
        double normalizedOffset;        // offset on [0..1)
    };

    typedef QVector<TrackQueue> TrackQueueVectorT;
    
    EventQueue();
    ~EventQueue();

    void initialize( const Composition* );

    void restart( qint64 origin, qint64 now );
    void start( qint64 now =-1 );
    void stop( qint64 now =-1 );

    void advance( qint64 now, bool computeOffsets =false );

    void startTrack( const Track*, qint64 now =-1 );
    void stopTrack( const Track*, qint64 now =-1 );

    Event* takeFront( qint64 now =-1 );

    qint64 origin() const { return _origin; }
    qint64 now() const { return _now; }
    qint64 minTimeUntilNextEvent( qint64 max ) const;
    inline qint64 minTimeUntilNextEvent( qint64 max, qint64 now ) { _now=now; return minTimeUntilNextEvent( max ); }
    inline qint64 elapsedTime() const { return _now - _origin; }

    inline const TrackQueueVectorT& tracks() const { return _tracks; }

private:
    void addTrack( const Track* t, const Composition* );
    TrackQueue* find( const Track* );
    void startQueue( TrackQueue* );
    void stopQueue( TrackQueue* );
    qint64 elapsedTrackTime( const TrackQueue* ) const;

    TrackQueueVectorT _tracks;
    qint64 _origin;
    qint64 _now;

};
