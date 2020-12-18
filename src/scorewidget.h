/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

#pragma once

#include <QWidget>

class Composition;
class PlayHead;

class ScoreWidget : public QWidget {
Q_OBJECT
public:
    ScoreWidget( QWidget *parent =0 );
    ~ScoreWidget();

    void setComposition( Composition* );
    const Composition* composition() const { return _comp; }

    void setPlayHead( const PlayHead* );
    const PlayHead* playHead() const { return _playhead; };

    void setDarkPalette( bool dark );
    bool hasDarkPalette() const { return _dark; }
    
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void paintEvent( QPaintEvent *event ) override;


private:
    Composition* _comp;
    const PlayHead* _playhead;
    bool _dark;
    QRect _window;
};
