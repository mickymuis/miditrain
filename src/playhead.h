/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

#pragma once

#include "composition.h"
#include <QVector>
//#include <QMultiMap>


class PlayHead {
public:
    struct Position {
        qint64 length, offset;          // track length and offset in msec
        int lap;                        // n-th round
        double normalizedOffset;        // offset on [0..1)
        const Track* track;             // pointer to Track object
    };
    typedef QVector<Position> PositionVectorT;
    static Position InvalidPosition;

    PlayHead();
    ~PlayHead();

    void initialize( const Composition* );

    void restart( qint64 origin, qint64 now );

    void addTrack( const Track*, const Composition* );
    void removeTrack( const Track* );
    void removeTrack( int trackIndex );

    Position getPosition( int trackIndex ) const;
    Position getPosition( const Track* ) const;

    void advanceTo( qint64 now );
    
    inline qint64 origin() const { return _origin; }
    inline qint64 now() const { return _now; }
    inline qint64 elapsedTime() const { return _now - _origin; }

private:
    PositionVectorT _positions;
    qint64 _origin, _now;

};
Q_DECLARE_METATYPE( PlayHead )
