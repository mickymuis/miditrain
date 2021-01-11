/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

#include "playthread.h"
#include "composition.h"
#include <QMidiOut.h>
#include <QTimer>
#include <QMidiFile.h>
#include <QElapsedTimer>
#include <cstdio>

PlayThread::PlayThread( QObject *parent ) :
    QThread( parent ),
    _comp( nullptr ),
    _midiout( nullptr ) {
}

PlayThread::~PlayThread() {
}

void 
PlayThread::setComposition( const Composition* comp ) {
    if( isRunning() ) return;
    _comp =comp;
    //_playhead.reinitialize( comp );
    _queue.initialize( comp );

    // Move to class PlayHead in the future
    
    // Initialize the queue
    
/*    _eventq.clear();
    _sectionq.clear();

    for( int i =0; i < _comp->tracks().count(); i++ ) {
        const Track& t =_comp->tracks()[i];
        if( t.sections.isEmpty() ) continue;
        _sectionq[i] = 0; // track index = section index
    }*/
}

void 
PlayThread::setPlayHead( const PlayHead& ph ) {
    if( isRunning() ) return;
    _playhead =ph;
}

void 
PlayThread::setMidiOut( QMidiOut* midiout ) {
    if( isRunning() ) return;
    _midiout =midiout;
}

void 
PlayThread::setTimer( const QElapsedTimer& t ) {
    _timer =t;
    _queue.restart( 0, t.elapsed() );
}

void 
PlayThread::setStartTime( qint64 origin, qint64 now ) {
    _queue.restart( origin, now );
}

void 
PlayThread::run() {
    if( _comp == nullptr || _midiout == nullptr ) {
        exit(0);
        return;
    }
    //QElapsedTimer elapsed;
    //elapsed.start();
    //_previous =timeNow();
    //int tick =0;
    while( 1 ) {
        if( isInterruptionRequested() ) {
            _midiout->controlChange( 0, 120, 0 );
            exit(0);
            return;
        }
        //TimeVarT now = timeNow();
        //_playhead.advance( (double)elapsed.restart() );
//        _queue.advance( _timer.elapsed() );
        //_previous =now;

/*        for( const auto & track : _comp->tracks() ) {
            const PlayHead::Position& pos =_playhead.getPosition( track.index() );
            for( auto it =pos.events.begin(); it != pos.events.end(); it++ ) {
                if( ( pos.prevAngle < pos.angle && pos.prevAngle <= it.key() && pos.angle > it.key() ) 
                 || ( pos.prevAngle > pos.angle && pos.angle >= it.key() ) ) {
                    //printf( "Event %d\n ", it.value()->type() );
                    _midiout->sendEvent( *(it.value()) );
                }
            }


        }

//        if( tick % BROADCAST_DIVISION )
//            emit positionAdvanced( _playhead );

        msleep( PRECISION );
        tick++;*/

        EventQueue::Event* e =nullptr;
        while( (e = _queue.takeFront( _timer.elapsed() ) ) ) {
            _midiout->sendEvent( *e->midi );
        }

        msleep( _queue.minTimeUntilNextEvent( MAX_IDLE, _timer.elapsed() ) );
        //msleep( 1 );
    }
}

