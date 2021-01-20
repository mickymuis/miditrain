/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

#include "eventqueue.h"
#include <QMidiFile.h>
#include "composition.h"
#include <cstdio>

EventQueue::EventQueue() {}
EventQueue::~EventQueue() {
    clear();
}

void
EventQueue::clear() {
    for( auto tq : _tracks ) {
        delete tq;
    }
    _tracks.clear();
}

/** (Re-)populate the event queue with all events from @comp */
void 
EventQueue::initialize( const Composition* comp ) {

    clear();

    for( const auto& t : comp->tracks() ) {
        addTrack( &t, comp );
    }
}

void 
EventQueue::addTrack( const Track* t, const Composition* comp ) {
    EventVectorT events;

    // Tracklength in msec
    qint64 length =(qint64)((t->length() / t->tempo()) * 1000.0);
    
    TrackQueue* tq = new TrackQueue( { length, 0, t, EventVectorT(), 0, 0, t->autoStart(), false, 0, 0, 0. } );
    
    // We make a vector of all events in this track, starting with the 'loop begin'
    events.append( {LoopBeginEvent, 0, nullptr, nullptr, nullptr, tq} );
    
    // We want to 'flatten' all possible (midi) events for each section of this track
    for( const auto & sec : t->sections() ) {
        // Obtain the trigger for this section
        const Trigger* trig = comp->triggerById( sec.trigger );
        if( trig == nullptr ) continue;

        // We have to add duplicates for each axle, given its offset
        double axle =0.0;
        for( int i=0; i < t->axleCount(); i++ ) {
            axle += i==0 ? 0.0 : t->axleOffsets()[i-1];

            // Add the trigger's (midi) events with their absolute offsets
            for( const auto &event : trig->events() ) {
                
                const double angle =sec.offset + axle;;
                // Compute the timestamp from the angle
                const qint64 ts = (angle / t->tempo()) * 1000.0;
                if( event.type == Trigger::MidiEvent ) {
                    // For MIDI events we have to add the extra delay parameter
                    Event e ={TriggerEvent, (ts + event.midiDelay) % length, &event, trig, &sec, tq};
                    events.append( e );

                    if( event.midiDuration > 0 ) {
                        // If 'duration' is specified, we add an automagic NoteOff
                        Event e ={ImplicitNoteOffEvent, (ts + event.midiDelay + event.midiDuration) % length, &event, trig, &sec, tq};
                        events.append( e );
                    }

                } else if( i ==0 ) {
                    // Other events are only added for the first axle
                    Event e ={TriggerEvent, ts % length, &event, trig, &sec, tq};
                    events.append( e );
                }

            }
        }
    }

    std::sort( events.begin(), events.end(), [](const Event& a, const Event& b){ return a.timestamp < b.timestamp; } );
    tq->events =events;
    _tracks.append( tq );
}

void 
EventQueue::restart( qint64 origin, qint64 now ) {
    _origin =origin; _now =now;
    for( auto& tq : _tracks ) {
        // Find the first event that has its timestamp in the future
        tq->startTime =_now;
        tq->running =tq->start =tq->track->autoStart();
        int i =0;
        for( ; i < tq->events.count(); i++ ) {
            if( tq->events[i].timestamp >= elapsedTrackTime( tq ) % tq->length )
                break;
        }
        tq->cursor =i == tq->events.count() ? 0 : i;
        tq->lap =elapsedTrackTime( tq ) / tq->length;
        //startTrack( tq );
    }
}

void 
EventQueue::start( qint64 now ) {
    if( now != -1 ) _now = now;
    for( auto tq : _tracks ) {
        if( tq->start ) startTrack( tq );
    }

}

void 
EventQueue::stop( qint64 now ) {
    if( now != -1 ) _now = now;
    for( auto& tq : _tracks ) {
        tq->start = tq->running;
        stopTrack( tq );
    }
}

void 
EventQueue::startTrack( const Track* t, qint64 now ) {
    TrackQueue* tq =find( t );
    startTrack( tq, now );
}

void 
EventQueue::startTrack( EventQueue::TrackQueue* tq, qint64 now ) {
    if( !tq || tq->running ) return;
    if( now != -1 ) _now = now;
    tq->running =true;
    tq->startTime =_now;
}

void 
EventQueue::stopTrack( const Track* t, qint64 now  ) {
    TrackQueue* tq =find( t );
    stopTrack( tq, now );
}

void 
EventQueue::stopTrack( EventQueue::TrackQueue* tq, qint64 now ) {
    if( !tq || !tq->running ) return;
    if( now != -1 ) _now = now;
    tq->running =false;
    tq->runningTime += _now - tq->startTime;
}

void 
EventQueue::resetTrack( const Track* t, qint64 now  ) {
    TrackQueue* tq =find( t );
    resetTrack( tq, now );
}

void 
EventQueue::resetTrack( EventQueue::TrackQueue* tq, qint64 now ) {
    if( !tq ) return;
    if( now != -1 ) _now = now;
    tq->running =tq->track->autoStart();
    tq->runningTime =0;
    tq->startTime =tq->running ? _now : 0;
    tq->offset =0; tq->normalizedOffset =0.;
    tq->lap =0; tq->cursor =0;
}

void 
EventQueue::advance( qint64 now, bool computeOffsets ) {
    _now =now;
    if( computeOffsets ) {
        for( auto tq : _tracks ) {
            tq->offset = elapsedTrackTime( tq ) % tq->length;
            // tq->lap = elapsedTrackTime( tq ) / tq->length;
            tq->normalizedOffset = (double)tq->offset / (double)tq->length;
        }
    }
}

EventQueue::Event* 
EventQueue::takeFront( qint64 now ) {
    if( now != -1 ) _now = now;
    // Find the first track that has an event due
    for( auto tq : _tracks ) {
        if( tq->cursor >= tq->events.count() ) continue;
        if( tq->running == false ) continue;

        qint64 timestamp = tq->events[tq->cursor].timestamp;
        qint64 elapsedLap =elapsedTrackTime( tq ) % tq->length;
        qint64 lap =elapsedTrackTime( tq ) / tq->length;
        // The current event must be due in the current lap
        if( timestamp <= elapsedLap
            && tq->lap == lap ) { 
            Event* e =&tq->events[tq->cursor];
            // Increment the cursor and the lap number if needed
            if( ++tq->cursor == tq->events.count() ) {
                tq->cursor =0;
                tq->lap++;
            }
            return e;
        }
    }
    return nullptr;
}

qint64 
EventQueue::minTimeUntilNextEvent( qint64 max ) const {
    qint64 time =max;
    // Find the track that has an event due in the shortest amount of time
    for( const auto tq : _tracks ) {
        if( tq->cursor >= tq->events.count() ) continue;
        if( tq->running == false ) continue;

        qint64 timestamp = tq->events[tq->cursor].timestamp;
        qint64 elapsedLap =elapsedTrackTime( tq ) % tq->length;
        qint64 lap =elapsedTrackTime( tq ) / tq->length;

        if( tq->cursor == 0 && tq->lap == lap + 1 )
            time =qMin( time, timestamp + (tq->length - elapsedLap) );
        else if( timestamp >= elapsedLap
                 && tq->lap == lap )
            time =qMin( time, tq->events[tq->cursor].timestamp - elapsedLap );
    }
    return time;
}

EventQueue::TrackQueue* 
EventQueue::find( const Track* t ) {
    for( auto tq : _tracks ) {
        if( tq->track == t ) return tq;
    }
    return nullptr;
}

qint64 
EventQueue::elapsedTrackTime( const TrackQueue* tq ) const {
    if( !tq ) return 0;
    if( tq->running == false )
        return tq->runningTime;
    return tq->runningTime + (_now - tq->startTime);
}
