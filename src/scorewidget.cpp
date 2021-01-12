/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

#include "scorewidget.h"
#include "composition.h"
#include "playhead.h"
#include <QPainter>
#include <QPen>
#include <QRect>
#include <QtMath>
#include <cstdio>

ScoreWidget::ScoreWidget( QWidget* parent ) : QWidget( parent ), _comp( nullptr ) {
    
    setBackgroundRole( QPalette::Window );
    setAutoFillBackground( true );
    setDarkPalette( false );

}

ScoreWidget::~ScoreWidget() { }

void
ScoreWidget::setDarkPalette( bool dark ) {
    _dark =dark;

    QPalette pal;
    pal.setColor( QPalette::Window, dark ? Qt::black : Qt::white );
    pal.setColor( QPalette::WindowText, dark ? Qt::white : Qt::black );
}

void
ScoreWidget::setComposition( Composition* comp ) {
    _comp =comp;
    update();
}

void
ScoreWidget::setPlayHead( const PlayHead* ph ) {
    _playhead =ph;
    update();
}

QSize 
ScoreWidget::minimumSizeHint() const {
    return QSize( 400, 400 );
}

QSize 
ScoreWidget::sizeHint() const {
    return minimumSizeHint(); // for now
}

void 
ScoreWidget::paintEvent( QPaintEvent *event ) {
    if( _comp == nullptr || _comp->tracks().isEmpty() ) return;

    const float stroke =.02f;
    const float markerSize =.5f;
    //const int count = _comp->tracks().count();
    const float radiusStep = 1.f;
    const float margin = 1.f;
    const float windowRadius = 2.f + radiusStep * (_comp->tracks().count()-1) + margin;
    
    _window =QRect( -qCeil(windowRadius), -qCeil(windowRadius), qCeil(2 * windowRadius), qCeil(2 * windowRadius) );

    QPainter painter (this);
    painter.setWindow( _window );
    
    /* In addition, we set up the viewport to match the ratio of the window */

    float s =.0f;

    if( width() > height() ) {     
        float w = ((float)_window.width() / (float)_window.height()) * (float)height();
        int delta = (int)w - width();
        painter.setViewport( -delta / 2, 0, (int)w, height() );
        s = w / _window.width();
    } else {
        float h = ((float)_window.height() / (float)_window.width()) * (float)width();
        int delta = (int)h - height();
        painter.setViewport( 0, -delta / 2, width(), (int)h );
        s = h / _window.height();
    }

//    viewport = painter.viewport();

    painter.setRenderHint( QPainter::Antialiasing, true );
    
    QPen pen;
    pen.setColor( palette().color( QPalette::WindowText ) );
    pen.setWidthF( stroke );
    painter.setPen( pen );

    float radius = 1.f;

    for( auto track : _comp->tracks() ) {

        // Calculate the number of degrees of the circle that correspond to one window-space unit
        const float degPerUnit = 360.0 / (2.0 * radius * M_PI);
//        painter.drawEllipse( QPointF(0,0), radius, radius );
        const int length =track.length();
        const double c =360.0 / (double)length; // Factor to convert track length to degrees
        QRectF circle( -radius, -radius, radius*2, radius*2 );
        
        if( track.sections().isEmpty() ) {
            //painter.drawEllipse( circle );
            continue;
        }

        const int sects  =track.sections().count();
        const double pos =_playhead != nullptr ? _playhead->getPosition( track.index() ).normalizedOffset * length: 0.0;

        for( int i =0; i < sects; i++ ) {
            const float gap = degPerUnit * (markerSize/4.0); // degrees
            const Track::Section& section1 =track.sections()[i];
            const Track::Section& section2 =
                i == sects-1 
                    ? track.sections()[0] 
                    : track.sections()[i+1];
            const float angle1 =section1.offset;
            const float angle2 =(i == sects-1 ? length : 0.0) + section2.offset;
            
            // Determine if the playhead is in the current section
            if( pos >= angle1 && pos < angle2 )
                pen.setColor( Qt::red );
            else
                pen.setColor( palette().color( QPalette::WindowText ) );
            painter.setPen( pen );

            // Draw the arc first
            float start =c*angle1 - 90.0f; // -90 to start in the 12' oclock position
            float span  =(c*angle2 - 90.0f) - start - gap;
            painter.drawArc( circle, -start*16.f, -span*16.f ); 
            //printf( "Draw Arc %f, %f\n", -start, -span );
            
            const Trigger* trig = _comp->triggerById( section1.trigger );

            // Now draw the marker.
            // For a stop-event we use a square
            if( trig && trig->hasStopEvent() ) {
                double o =degPerUnit * markerSize;
                QPointF square[4] = {
                        QPointF( qCos( qDegreesToRadians(start) ) * (radius-markerSize/2.f),
                                 qSin( qDegreesToRadians(start) ) * (radius-markerSize/2.f) ),
                        QPointF( qCos( qDegreesToRadians(start) ) * (radius+markerSize/2.f),
                                 qSin( qDegreesToRadians(start) ) * (radius+markerSize/2.f) ), 
                        QPointF( qCos( qDegreesToRadians(start+o) ) * (radius+markerSize/2.f),
                                 qSin( qDegreesToRadians(start+o) ) * (radius+markerSize/2.f) ),
                        QPointF( qCos( qDegreesToRadians(start+o) ) * (radius-markerSize/2.f),
                                 qSin( qDegreesToRadians(start+o) ) * (radius-markerSize/2.f) ) };
                painter.drawPolygon( square, 4 );

            } else {
                // For a Midi-event we use a perpendicular line marking the section
                painter.drawLine(
                        QPointF( qCos( qDegreesToRadians(start) ) * (radius-markerSize/2.f),
                                 qSin( qDegreesToRadians(start) ) * (radius-markerSize/2.f) ),
                        QPointF( qCos( qDegreesToRadians(start) ) * (radius+markerSize/2.f),
                                 qSin( qDegreesToRadians(start) ) * (radius+markerSize/2.f) ) );
            }

        }

        // Draw the 'train'
        double o =0.0;
        for( int i =0; i < track.axleCount(); i++ ) {
            o -= i==0 ? 0.0 : track.axleOffsets()[i-1];
            double angle =c * (o+pos) - 90.0;
            QPointF train[3] = {
                    QPointF( qCos( qDegreesToRadians(angle) ) * (radius-markerSize/2.f),
                             qSin( qDegreesToRadians(angle) ) * (radius-markerSize/2.f) ),
                    QPointF( qCos( qDegreesToRadians(angle) ) * (radius+markerSize/2.f),
                             qSin( qDegreesToRadians(angle) ) * (radius+markerSize/2.f) ),
                    QPointF( qCos( qDegreesToRadians(angle+degPerUnit*markerSize) ) * (radius),
                             qSin( qDegreesToRadians(angle+degPerUnit*markerSize) ) * (radius) ) };
            pen.setColor( palette().color( QPalette::WindowText ) );
            painter.setPen( pen );
            painter.drawPolygon( train, 3 );
        }


        radius += radiusStep;
    }

}
