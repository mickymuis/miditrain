/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

#include "eventqueue.h"
#include <QMidiFile.h>
#include "composition.h"

EventQueue::EventQueue() {}
EventQueue::~EventQueue() {}

/** (Re-)populate the event queue with all events from @comp */
void 
EventQueue::initialize( const Composition* comp ) {

    _tracks.clear();

    for( const auto& t : comp->tracks() ) {
        addTrack( &t, comp );
    }
}

void 
EventQueue::addTrack( const Track* t, const Composition* comp ) {
    EventVectorT events;

    // Tracklength in msec
    qint64 length =(qint64)((t->length() / t->tempo()) * 1000.0);

    // We want to 'flatten' all possible midi events for each section of this track
    for( const auto & sec : t->sections() ) {
        // Obtain the trigger for this section
        const Trigger* trig = comp->triggerById( sec.on_enter );
        if( trig == nullptr ) continue;

        // We have to add duplicates for each axle, given its offset
        double axle =0.0;
        for( int i=0; i < t->axleCount(); i++ ) {
            axle += i==0 ? 0.0 : t->axleOffsets()[i-1];


            // Add the trigger's midi events with their absolute offsets
            for( const auto &event : trig->events() ) {
                
                const double angle =sec.offset + axle;;
                // Compute the timestamp from the angle
                const qint64 ts = (angle / t->tempo()) * 1000.0 + event.delay;
                
                Event e ={ts % length, &event.midiEvent, trig};
                events.append( e );
            }
        }
    }

    std::sort( events.begin(), events.end(), [](const Event& a, const Event& b){ return a.timestamp < b.timestamp; } );

    TrackQueue q ={ length, t, events, 0, 0 };
    _tracks.append( q );
}

void 
EventQueue::restart( qint64 origin, qint64 now ) {
    _origin =origin; _now =now;
    for( auto& tq : _tracks ) {
        // Find the first event that has its timestamp in the future
        int i =0;
        for( ; i < tq.events.count(); i++ ) {
            if( tq.events[i].timestamp >= elapsedTime() % tq.length )
                break;
        }
        tq.cursor =i == tq.events.count() ? 0 : i;
        tq.lap =elapsedTime() / tq.length;
    }
}

void 
EventQueue::advance( qint64 now ) {
    _now =now;
}

EventQueue::Event* 
EventQueue::takeFront( qint64 now ) {
    if( now != -1 ) _now = now;
    // Find the first track that has an event due
    for( auto& tq : _tracks ) {
        if( tq.cursor >= tq.events.count() ) continue;

        qint64 timestamp = tq.events[tq.cursor].timestamp;
        qint64 elapsedLap =elapsedTime() % tq.length;
        qint64 lap =elapsedTime() / tq.length;
        // The current event must be due in the current lap
        if( timestamp <= elapsedLap
            && tq.lap == lap ) {
            Event* e =&tq.events[tq.cursor];
            // Increment the cursor and the lap number if needed
            if( ++tq.cursor == tq.events.count() ) {
                tq.cursor =0;
                tq.lap++;
            };
            return e;;
        }
    }
    return nullptr;
}



qint64 
EventQueue::minTimeUntilNextEvent( qint64 max ) const {
    qint64 time =max;
    // Find the track that has an event due in the shortest amount of time
    for( const auto& tq : _tracks ) {
        if( tq.cursor >= tq.events.count() ) continue;

        qint64 timestamp = tq.events[tq.cursor].timestamp;
        qint64 elapsedLap =elapsedTime() % tq.length;
        qint64 lap =elapsedTime() / tq.length;

        if( tq.cursor == 0 && tq.lap == lap + 1 )
            time =qMin( time, timestamp + (tq.length - elapsedLap) );
        else if( timestamp >= elapsedLap
                 && tq.lap == lap )
            time =qMin( time, tq.events[tq.cursor].timestamp - elapsedLap );
    }
    return time;
}
