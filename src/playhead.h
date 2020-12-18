/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

#pragma once

#include "composition.h"
#include <QVector>
#include <QMultiMap>


class PlayHead {
public:
    typedef QMultiMap<double /*angle*/,const QMidiEvent*> EventVectorT;

    struct Position {
        const Track* track;
        double angle, prevAngle;
        EventVectorT events;
    };
    typedef QVector<Position> PositionVectorT;
    static Position InvalidPosition;

    PlayHead();
    ~PlayHead();

    void reinitialize( const Composition* );

    void addTrack( const Track*, const Composition* );
    void removeTrack( const Track* );
    void removeTrack( int trackIndex );

    Position getPosition( int trackIndex ) const;
    Position getPosition( const Track* ) const;

    void advance( double msecElapsed );

private:
    PositionVectorT _positions;

};
Q_DECLARE_METATYPE( PlayHead )
