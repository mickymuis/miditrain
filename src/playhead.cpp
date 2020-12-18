/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

#include "playhead.h"
#include <cstdio>

PlayHead::Position PlayHead::InvalidPosition = { nullptr, 0.0, 0.0, PlayHead::EventVectorT() };

PlayHead::PlayHead() {}
PlayHead::~PlayHead() {}

void 
PlayHead::reinitialize( const Composition* comp ) {
    _positions.clear();

    for( auto & t : comp->tracks() ) {
        addTrack( &t, comp );
    }
}

void 
PlayHead::addTrack( const Track* t, const Composition* comp ) {
    Position p;
    p.track =t;
    p.angle =.0;
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
                
                double angle =sec.offset + axle + (t->tempo() * (event.delay / 1000.0));
                p.events.insert( angle, &event.midiEvent );
            }
        }
    }
    _positions.append( p );
}

void 
PlayHead::removeTrack( const Track* t ) {
    for( auto it =_positions.begin(); it != _positions.end(); it++ ) {
        if( (*it).track == t ) {
            _positions.erase( it );
            break;
        }
    }
}

void 
PlayHead::removeTrack( int trackIndex ) {
    for( auto it =_positions.begin(); it != _positions.end(); it++ ) {
        if( (*it).track->index() == trackIndex ) {
            _positions.erase( it );
            break;
        }
    }
}

PlayHead::Position 
PlayHead::getPosition( int trackIndex ) const {
    for( auto p : _positions ) {
        if( p.track->index() == trackIndex ) {
            return p;
        }
    }
    return InvalidPosition;
}

PlayHead::Position 
PlayHead::getPosition( const Track* t ) const {
    for( auto p : _positions ) {
        if( p.track == t ) {
            return p;
        }
    }
    return InvalidPosition;
}

void 
PlayHead::advance( double msecElapsed ) {
    for( auto & p : _positions ) {
        double delta =(p.track->tempo() * msecElapsed) / 1000.0;
        p.prevAngle =p.angle;
        p.angle += delta;
        //printf( "advancing by %f\n", delta );
        if( p.angle > (double)p.track->length() ) p.angle -=(double)p.track->length();
    }
}


