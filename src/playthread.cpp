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
            // TODO: keep track of actually used channels
            allNotesOff();
        //    for( int i =0; i < 16; i++ )
        //        _midiout->controlChange( i, 120, 0 );
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
            midi.setNote( midi.note() + e->section->transpose );
            
            // Update the list of sounding midi notes for NoteOn and NoteOff events
            if( e->event->midiEvent.type() == QMidiEvent::NoteOn )
                _amnotes[e->trackQueue][midi.voice()][midi.note()]++;
            else if( e->event->midiEvent.type() == QMidiEvent::NoteOff ) {
                trackNoteOff( e->trackQueue, midi.voice(), midi.note(), midi.velocity() );
                break;
            }
            
            _midiout->sendEvent( midi );

            break;
        }
        case Trigger::StopEvent:
            allTrackNotesOff( e->trackQueue );
            _queue.stopTrack( e->trackQueue );
            break;
        case Trigger::StartEvent:
            _queue.startTrack( _comp->trackById( e->event->target ) );
            break;
        case Trigger::ResetEvent: {
            EventQueue::TrackQueue* tq = _queue.find( _comp->trackById( e->event->target ) );
            if( !tq ) break;
            allTrackNotesOff( tq );
            _queue.resetTrack( tq );
            break;
        }
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
        midi.setNote( midi.note() + e->section->transpose );
        //midi.setType( QMidiEvent::NoteOff );
        //_midiout->sendEvent( midi );
        trackNoteOff( e->trackQueue, midi.voice(), midi.note(), midi.velocity() );
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

void PlayThread::debug() {
    printf( "\n" );

    for( auto it = _amnotes.begin(); it != _amnotes.end(); it++ ) {
        const EventQueue::TrackQueue* tq =it.key();
        for( auto it2 = it.value().begin(); it2 != it.value().end(); it2++ ) {
            int c = it2.key();
            for( auto it3 = it2.value().begin(); it3 != it2.value().end(); it3++ ) {
                printf( "[%d] channel %d: note: %d count %d\n", tq->track->id(), c, it3.key(), it3.value() );
                for( int i=0; i < it3.value(); i++ ) {
                    _midiout->noteOff( it3.key(), c, 0 );
                }
            }
            it2.value().clear();
        }
        it.value().clear();
    }
    _amnotes.clear();
}


/** Send note-off events for all notes played from all tracks.
 * If the same note is activated multiple times, an identical amount of note-offs will be sent.*/
void PlayThread::allNotesOff() {
//    printf( "\n" );

    // We iterate over all notes activated on each channel by each track
    // ... so this is what a triple nested STL-style iterator looks like ...
    for( auto it = _amnotes.begin(); it != _amnotes.end(); it++ ) {
        const EventQueue::TrackQueue* tq =it.key();
        for( auto it2 = it.value().begin(); it2 != it.value().end(); it2++ ) {
            int c = it2.key();
            for( auto it3 = it2.value().begin(); it3 != it2.value().end(); it3++ ) {
                //printf( "[%d] channel %d: note: %d count %d\n", tq->track->id(), c, it3.key(), it3.value() );
                for( int i=0; i < it3.value(); i++ ) {
                    _midiout->noteOff( it3.key(), c, 0 );
                }
            }
            it2.value().clear();
        }
        it.value().clear();
    }
    _amnotes.clear();
}
    
void 
PlayThread::allTrackNotesOff( const EventQueue::TrackQueue* tq ) {
    // Check if this track has any used channels, if not return
    if( !_amnotes.contains(tq) || _amnotes[tq].isEmpty() ) return;

    // Iterate over all channels for this track
    for( auto it = _amnotes[tq].begin(); it != _amnotes[tq].end(); it++ ) {
        for( auto it2 = it.value().begin(); it2 != it.value().end(); it2++ ) {
            int n =it2.key(), c = it.key();
            while( it2.value() > 0 ) {
           
                _midiout->noteOff( n, c, 0 );
                _amnotes[tq][c][n]--;
            }
        }
        it.value().clear();
    }
    _amnotes[tq].clear();
}


/** Kill one note played from the track-queue @tq
 * if @all is true, multiple note-off events are sent if identical notes have been activated more than once.
 */
void 
PlayThread::trackNoteOff( const EventQueue::TrackQueue* tq, int channel, int note, int velocity, bool all ) {

    int n = note, c = channel;
    // Check if this track has used notes in @channel
    if( _amnotes.contains(tq) &&  _amnotes[tq].contains( c ) ) {
 
            if( _amnotes[tq][c].contains( n ) && _amnotes[tq][c][n] > 0 ) {
                do {
                    _midiout->noteOff( n, c, velocity );
                    _amnotes[tq][c][n]--;
                } while( all && _amnotes[tq][c][n] > 0 );
                if( _amnotes[tq][c][n] == 0 )
                    _amnotes[tq][c].remove( n );
            }
        
            if( _amnotes[tq][c].isEmpty() )
                _amnotes[tq].remove( c );

    }
}
