/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

#include "playhead.h"
#include <cstdio>

PlayHead::Position PlayHead::InvalidPosition = { 0, 0, 0, 0., nullptr };

PlayHead::PlayHead() {}
PlayHead::~PlayHead() {}

void 
PlayHead::initialize( const Composition* comp ) {
    _positions.clear();

    for( auto & t : comp->tracks() ) {
        addTrack( &t, comp );
    }
}

void 
PlayHead::restart( qint64 origin, qint64 now ) {
     _origin =origin;
    advanceTo( now );
}

void 
PlayHead::addTrack( const Track* t, const Composition* comp ) {
    // Tracklength in msec
    qint64 length =(qint64)((t->length() / t->tempo()) * 1000.0);
    
    Position p { length, 0, 0, 0., t };
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
PlayHead::removeTrack( int trackId ) {
    for( auto it =_positions.begin(); it != _positions.end(); it++ ) {
        if( (*it).track->id() == trackId ) {
            _positions.erase( it );
            break;
        }
    }
}

PlayHead::Position 
PlayHead::getPosition( int trackId ) const {
    for( auto p : _positions ) {
        if( p.track->id() == trackId ) {
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
PlayHead::advanceTo( qint64 now ) {
    _now =now;
    for( auto & p : _positions ) {
        p.offset = elapsedTime() % p.length;
        p.lap = elapsedTime() / p.length;
        p.normalizedOffset = (double)p.offset / (double)p.length;
    }
}


