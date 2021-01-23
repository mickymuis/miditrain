/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

#include "mainwindow.h"
#include "scorewidget.h"
#include "composition.h"
#include "playthread.h"
#include "eventqueue.h"

#include <QFile>
#include <QMessageBox>
#include <QAction>
#include <QMenuBar>
#include <QMidiOut.h>
#include <cstdio>

MainWindow::MainWindow() : 
    QMainWindow(), 
    //_playhead( nullptr ),
    _composition( nullptr ), 
    _thread( nullptr ),
    _midiout( nullptr ),
    _playing( false ),
    _restart( true ) { 

    resize( 1000, 1000 );

    _scoreWidget =new ScoreWidget( this );
    _scoreWidget->setEventQueue( &_queue );
    //_scoreWidget->setPlayHead( _playhead );


    setCentralWidget( _scoreWidget );

    QMenu* fileMenu =menuBar()->addMenu( tr("&File" ) );
    QMenu* playbackMenu =menuBar()->addMenu( tr("&Playback" ) );


    QAction* playbackAct =new QAction( tr("Play/pause"), this );
    playbackAct->setShortcut( QKeySequence( Qt::Key_Space ) );
    connect( playbackAct, &QAction::triggered, this, &MainWindow::togglePlayback );
    playbackMenu->addAction( playbackAct );

    _midiout =new QMidiOut();
    auto devices =_midiout->devices();
    for( auto it =devices.begin(); it != devices.end(); it++ ) {
        printf( "'%s'\t:\t'%s'\n", qPrintable( it.key() ), qPrintable( it.value() ) );
    }

    if( !devices.isEmpty() ) {
        _midiout->connect( devices.begin().key() );
    }
    if( !_midiout->isConnected() )
        QMessageBox::critical( this, this->windowTitle(), "Could not connect to a MIDI device." );
    else
        printf( "Connected to '%s'\n", qPrintable( _midiout->deviceId() ) );

//    _midiout->setInstrument( 1, 1 );
    //_midiout->noteOn( 64, 0, 60 );
    
   // _midiout->noteOn( 60, 0, 60 );

    _thread =new PlayThread( this );
    _thread->setMidiOut( _midiout );
    //qRegisterMetaType<PlayHead>();
    //connect( _thread, &PlayThread::positionAdvanced, this, &MainWindow::updatePosition );

    _timer =new QTimer( this );
    connect( _timer, &QTimer::timeout, this, &MainWindow::tick );
    
    _time.start();
    _thread->setTimer( _time );
}

MainWindow::~MainWindow() { 
    stop();
    _thread->requestInterruption();
    _thread->quit();
    _thread->wait();
    delete _thread;
    //delete _playhead;
}

bool 
MainWindow::openJsonFile( const QString& path ) {
    if( _composition != nullptr ) {
        // Maybe we should ask to save the file?
    }
    QFile file( path );
    if( !file.open( QIODevice::ReadOnly | QIODevice::Text) ) {
        QMessageBox::critical( this, this->windowTitle(), "Could not open given file for reading." );
        return false;
    }

    QString err;
    Composition *comp =new Composition();
    *comp =Composition::fromJson( file.readAll(), &err );

    if( !comp->isValid() ) {
        QMessageBox::critical( this, this->windowTitle(), err );
        delete comp;
        return false;
    }

    setComposition( comp );

    return true;
}
    
void 
MainWindow::setComposition( Composition* comp ) {
    if( _composition != nullptr ) {
        if( _playing ) stop();
        delete _composition;
    }
    _composition =comp;
    //_playhead->initialize( comp );
    _queue.initialize( comp );
    _scoreWidget->setComposition( comp );
    _thread->setComposition( comp );
}

void
MainWindow::togglePlayback() {
    if( _playing ) stop(); 
    else start();
}

void 
MainWindow::start() {
    if( _playing || _composition == nullptr) return;
    _playing =true;

    qint64 t =_time.elapsed();
    if( _restart ) {
        //_playhead->restart( t, t );
        _queue.restart( t, t );
        _thread->queue().restart( t, t );
    } else {
        //qint64 d =_stoptime - _queue.origin();
        //_playhead->restart( t - d, t );
        //_queue.restart( t - d, t );
        //_thread->setStartTime( t - d, t );
        _queue.start( t );
        _thread->queue().start( t );
    }
    _thread->start(QThread::HighPriority);
    _tick =0;
    _timer->start( UPDATE_PRECISION );
}

void 
MainWindow::stop() {
    if( !_playing ) return;
    _playing =false;

    _timer->stop();
    qint64 t =_time.elapsed();
    _thread->requestInterruption();
    _stoptime =t;
    _restart =false;

    _queue.stop( t );
    while( _thread->isRunning() ) {}
    _thread->queue().stop( t );
//    for( int i =0; i < 16; i++ )
//        _midiout->controlChange( i, 123, 0 );
//    _thread->debug();
}

/*void 
MainWindow::updatePosition( PlayHead ph ) {
   // *_playhead =ph;
    _scoreWidget->update(); 
}*/

    
void 
MainWindow::tick() {
    _queue.advance( _time.elapsed(), true );

    bool debug =false;

    EventQueue::Event* e =nullptr;
    while( (e = _queue.takeFront()) ) {
        if( debug ) printf( "%lld: ", _queue.now() );
        switch( e->type ) {
        case EventQueue::TriggerEvent:
            if( debug ) printf( "(TRIGGER) " );
            switch( e->event->type ) {
            case Trigger::MidiEvent:
                if( debug ) printf( "Midi Event " );
                break;
            case Trigger::StopEvent:
                if( debug ) printf( "Stop " );
                _queue.stopTrack( e->trackQueue );
                break;
            case Trigger::StartEvent:
                if( debug ) printf( "Start " );
                _queue.startTrack( _composition->trackById( e->event->target ) );
                break;
            case Trigger::ResetEvent:
                if( debug ) printf( "Reset " );
                _queue.resetTrack( _composition->trackById( e->event->target ) );
                break;
            case Trigger::NoEvent:
            default:
                break;
            }
            
            break;
        case EventQueue::LoopBeginEvent: {
            if( debug ) printf( "(LOOPBEGIN) " );
            int maxLoop = e->trackQueue->track->loopCount();
            if( maxLoop > 0 && maxLoop == e->trackQueue->lap ) {
                _queue.stopTrack( e->trackQueue );
            }
            break;
            }
        case EventQueue::ImplicitNoteOffEvent:
            if( debug ) printf( "(IMPLICITNOTEOFF) " );
        default: break;
        } 
        if( debug ) printf( "\n" );
    }
    
    if( (_tick % DISPLAY_PRECISION) == 0 )
        _scoreWidget->update();

    _tick += UPDATE_PRECISION;
}
