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
    _queue.initialize( comp );

}

/*void 
PlayThread::setPlayHead( const PlayHead& ph ) {
    if( isRunning() ) return;
    _playhead =ph;
}*/

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

/*void 
PlayThread::setStartTime( qint64 origin, qint64 now ) {
    _queue.restart( origin, now );
}*/

void 
PlayThread::run() {
    if( _comp == nullptr || _midiout == nullptr ) {
        exit(0);
        return;
    }
    while( 1 ) {
        if( isInterruptionRequested() ) {
            _midiout->controlChange( 0, 120, 0 );
            exit(0);
            return;
        }

//        if( tick % BROADCAST_DIVISION )
//            emit positionAdvanced( _playhead );

        EventQueue::Event* e =nullptr;
        while( (e = _queue.takeFront( _timer.elapsed() ) ) ) {
           processEvent( e );
        }

        msleep( _queue.minTimeUntilNextEvent( MAX_IDLE, _timer.elapsed() ) );
        //msleep( 1 );
    }
}

void
PlayThread::processEvent( const EventQueue::Event* e ) {
    switch( e->type ) {
    case EventQueue::TriggerEvent:
        if( !e->event ) break;

        switch( e->event->type ) {
        case Trigger::MidiEvent: {
            QMidiEvent midi =e->event->midiEvent;
            if( midi.voice() == -1 )
                midi.setVoice( e->trackQueue->track->midiChannel() );
            _midiout->sendEvent( midi );
            break;
        }
        case Trigger::StopEvent:
            _queue.stopTrack( e->trackQueue );
            break;
        case Trigger::StartEvent:
            _queue.startTrack( _comp->trackById( e->event->target ) );
            break;
        case Trigger::ResetEvent:
            _queue.resetTrack( _comp->trackById( e->event->target ) );
            break;
        case Trigger::NoEvent:
        default:
            break;
        };
        break;

    case EventQueue::ImplicitNoteOffEvent: {
        if( e->event->midiEvent.type() != QMidiEvent::NoteOn ) break;
        QMidiEvent midi =e->event->midiEvent;
        if( midi.voice() == -1 )
            midi.setVoice( e->trackQueue->track->midiChannel() );
        midi.setType( QMidiEvent::NoteOff );
        _midiout->sendEvent( midi );
        break;
    }
    case EventQueue::LoopBeginEvent: {
        int maxLoop = e->trackQueue->track->loopCount();
        if( maxLoop > 0 && maxLoop == e->trackQueue->lap )
            _queue.stopTrack( e->trackQueue );
        break;
        }
    };

}

