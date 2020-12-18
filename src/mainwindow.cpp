/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

#include "mainwindow.h"
#include "scorewidget.h"
#include "composition.h"
#include "playhead.h"
#include "playthread.h"

#include <QFile>
#include <QMessageBox>
#include <QAction>
#include <QMenuBar>
#include <QMidiOut.h>
#include <cstdio>

MainWindow::MainWindow() : 
    QMainWindow(), 
    _playhead( nullptr ),
    _composition( nullptr ), 
    _thread( nullptr ),
    _midiout( nullptr ),
    _playing( false ) { 
    _playhead =new PlayHead();

    _scoreWidget =new ScoreWidget( this );
    _scoreWidget->setPlayHead( _playhead );

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
    qRegisterMetaType<PlayHead>();
    connect( _thread, &PlayThread::positionAdvanced, this, &MainWindow::updatePosition );

    _timer =new QTimer( this );
    connect( _timer, &QTimer::timeout, this, &MainWindow::tick );

}

MainWindow::~MainWindow() { 
    stop();
    _thread->requestInterruption();
    _thread->quit();
    _thread->wait();
    delete _thread;
    delete _playhead;
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
    _playhead->reinitialize( comp );
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

    _elapsed.start();
    _thread->start(QThread::HighPriority);
//    _previous = timeNow();
    _timer->start( DISPLAY_PRECISION );
}

void 
MainWindow::stop() {
    if( !_playing ) return;
    _playing =false;

    _timer->stop();
    _thread->requestInterruption();
    _midiout->controlChange( 0, 120, 0 );
}

void 
MainWindow::updatePosition( PlayHead ph ) {
    *_playhead =ph;
    _scoreWidget->update(); 
}

    
void 
MainWindow::tick() {
//    if (event->timerId() == _timer.timerId()) {
        //TimeVarT now = timeNow();
        _playhead->advance( (double)_elapsed.restart() );
        //_previous =now;

        //*_playhead =_thread->playHead();
        _scoreWidget->update();

/*        for( const auto & track : _composition->tracks() ) {
            const PlayHead::Position& pos =_playhead->getPosition( track.index() );
            for( auto it =pos.events.begin(); it != pos.events.end(); it++ ) {
                if( ( pos.prevAngle < pos.angle && pos.prevAngle <= it.key() && pos.angle > it.key() ) 
                 || ( pos.prevAngle > pos.angle && pos.angle >= it.key() ) ) {
                    //printf( "Event %d\n ", it.value()->type() );
                    _midiout->sendEvent( *(it.value()) );
                }
            }
        }*/


  /*  } else {
        QWidget::timerEvent(event);
    }*/
}
