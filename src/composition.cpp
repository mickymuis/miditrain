/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

#include "composition.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <cstdio>

QMidiEvent::EventType
eventTypeFromStr( const QString& str ) {

    QMidiEvent::EventType t =QMidiEvent::Invalid;
    if( str.toLower() == "noteon" )
        t =QMidiEvent::NoteOn;
    else if( str.toLower() == "noteoff" )
        t =QMidiEvent::NoteOff;
    else if( str.toLower() == "keypressure" )
        t =QMidiEvent::KeyPressure;
    else if( str.toLower() == "channelpressure" )
        t =QMidiEvent::ChannelPressure;
    else if( str.toLower() == "controlchange" || str.toLower() == "cc" )
        t =QMidiEvent::ControlChange;
    else if( str.toLower() == "programchange" )
        t =QMidiEvent::ProgramChange;
    else if( str.toLower() == "pitchwheel" )
        t =QMidiEvent::PitchWheel;
    else if( str.toLower() == "meta" )
        t =QMidiEvent::Meta;
    else if( str.toLower() == "sysex" )
        t =QMidiEvent::SysEx;

    return t;
}

QString
eventTypeToStr( QMidiEvent::EventType t ) {
    QString str;
    switch( t ) {
        case QMidiEvent::NoteOn:
            str = "NoteOn";
            break;
        case QMidiEvent::NoteOff:
            str = "NoteOff";
            break;
        case QMidiEvent::KeyPressure:
            str = "KeyPressure";
            break;
        case QMidiEvent::ChannelPressure:
            str = "ChannelPressure";
            break;
        case QMidiEvent::ControlChange:
            str = "CC";
            break;
        case QMidiEvent::ProgramChange:
            str = "ProgramChange";
            break;
        case QMidiEvent::PitchWheel:
            str = "PitchWheel";
            break;
        case QMidiEvent::Meta:
            str = "Meta";
            break;
        case QMidiEvent::SysEx:
            str = "SysEx";
            break;
        case QMidiEvent::Invalid: break;
    }
    return str;
}


/* Class Trigger implementation */

Trigger::Trigger() : _id( -1 ), _hasStop( false ) { }
Trigger::~Trigger() { }

Trigger
Trigger::fromJson( const QJsonObject& json, QString* error ) {
    Trigger t;
    for( auto it =json.begin(); it != json.end(); it++ ) {
	if( it.key() == "Id" )
	    t.setId( it.value().toInt( t.id()/*default*/ ) );
        else if( it.key() == "Events" && it.value().isArray() ) {
            QJsonArray array = it.value().toArray();
            for( auto it2 =array.begin(); it2 != array.end(); it2++ ) {
                if( !t.addEventFromJson( (*it2).toObject(), error ) )
                    return t;
            }
        }
    }

    if( !t.isValid() ) {
        if( error != nullptr ) *error = "Incomplete trigger specification";
    }

    return t;
}

bool
Trigger::isValid() const {
    return _id != -1 && !_events.isEmpty();
}

void
Trigger::addEvent( const Event& e ) {
    _events.append( e );
    if( e.type == StopEvent )
        _hasStop =true;
}
    
bool 
Trigger::addEventFromJson( const QJsonObject& json, QString* error ) {
    QString etxt;
    Event e;

    QJsonValue jtype =json.value( "Type" );
    if( jtype.toString() == "Midi" ) {
        e.type = MidiEvent;
        e.midiDelay = json.value( "Delay" ).toInt( 0 );
        e.midiDuration = json.value( "Duration" ).toInt( 0 );
        e.midiEvent.setType( eventTypeFromStr( json.value( "Event" ).toString( "" ) ) );
        e.midiEvent.setNote( json.value( "Note" ).toInt( 60 ) );
        e.midiEvent.setVoice( json.value( "Channel" ).toInt( -1 ) );
        e.midiEvent.setVelocity( json.value( "Velocity" ).toInt( 60 ) );
        e.midiEvent.setValue( json.value( "Value" ).toInt( 0 ) );
        e.midiEvent.setNumber( json.value( "Number" ).toInt( 0 ) );
        if( e.midiEvent.type() == QMidiEvent::Invalid ) { etxt = "Invalid Midi"; goto ERROR; }

    } else if( jtype.toString() == "Stop" ) {
        e.type = StopEvent;

    } else if( jtype.toString() == "Start" ) {
        e.target = json.value( "Target" ).toInt( -1 );
        e.type = StartEvent;
    
    } else if( jtype.toString() == "Reset" ) {
        e.target = json.value( "Target" ).toInt( -1 );
        e.type = ResetEvent;

    } else { etxt = "No or incorrect type"; goto ERROR; }
    
    addEvent( e );
    return true;
ERROR:
    if( error != nullptr ) *error = "Incompletely specified trigger - " + etxt;
    return false;

}

/* Class Track implementation */

Track::Track()
    : _tempo( 0.0 ), _length( 360 ) { }
Track::~Track() { }

Track 
Track::fromJson( const QJsonObject& json, QString* error ) {
    Track t;

    t.setAutoStart( json.value( "Start" ).toBool( true ) );
    t.setMidiChannel( json.value( "MidiChannel" ).toInt( 0 ) );
    t.setId( json.value( "Id" ).toInt( -1 ) );
    t.setLoopCount( json.value( "Loop" ).toInt( 0 ) );
    t.setTempo( json.value( "Tempo" ).toDouble( 0.0 ) );
    t.setLength( json.value( "Length" ).toInt( 360 ) );
    if( !t.setOffsetsFromJson( json.value( "AxleOffsets" ).toArray(), error ) )
        return t;

    QJsonArray array = json.value( "Sections" ).toArray();

    for( auto it2 =array.begin(); it2 != array.end(); it2++ ) {
        if( !t.addSectionFromJson( (*it2).toObject(), error ) )
            return t;
    }

    if( !t.isValid() ) {
        if( error != nullptr ) *error = "Incomplete track specification";
    }
    return t;
}

bool
Track::isValid() const {
    return _tempo != 0.0 && !_sections.isEmpty();
}

bool
Track::setOffsetsFromJson( const QJsonArray& array, QString *error ) {
    _offsets.clear();

    for( auto it =array.begin(); it != array.end(); it++ ) {
        double offset = (*it).toDouble( 0.0 );
        if( offset == 0.0 ) {
            if( error != nullptr ) *error = "Incorrect axle offset specified";
            return false;
        }
        _offsets.append( offset );

    }
    return true;
}

bool 
Track::addSectionFromJson( const QJsonObject& json, QString *error ) {
    Section s;
    s.offset = json.value( "Offset" ).toDouble( -1 );
    s.trigger = json.value( "Trigger" ).toInt( -1 );
    s.transpose = json.value( "Transpose" ).toInt( 0 );
    
    if( s.offset < 0 ) goto ERROR;
    _sections.append( s );
    return true;
ERROR:
    if( error != nullptr ) *error = "Incompletely specified section";
    return false;
}


/* Class Composition implementation */



Composition::Composition() : _maxId(0) { }
Composition::~Composition() { }

Composition 
Composition::fromJson( const QByteArray& json, QString* error ) {
    Composition comp;
    QJsonParseError err;
    QJsonDocument doc =QJsonDocument::fromJson( json, &err );

    if( err.error != QJsonParseError::NoError ) {
        if( error != nullptr )
            *error = err.errorString() + " at " + QString::number( err.offset );
        return comp;
    }

    if( !doc.isObject() || doc.isEmpty() ) {
        if( error != nullptr ) *error = "Expected a root-level object in the document";
        return comp;
    }

    QJsonObject root =doc.object();

    for( auto it =root.begin(); it != root.end(); it++ ) {
        //printf( "Key: %s\n", qPrintable(it.key()) );
        if( it.key() == "Tracks" ) {
            if( !it.value().isArray() || comp.tracks().count() != 0 ) {
                if( error != nullptr ) *error = "\"tracks\" section must be specified as a single array";
                comp.clear();
                return comp;
            }
            QJsonArray array = it.value().toArray();
            for( auto it2 =array.begin(); it2 != array.end(); it2++ ) {
                Track t =Track::fromJson( (*it2).toObject(), error );
                if( !t.isValid() ) {
                    comp.clear();
                    return comp;
                }
                if( t.id() < 0 )
                    t.setId( ++comp._maxId );
                else
                    comp._maxId = qMax( comp._maxId, t.id() );
                comp._tracks.append( t );
            }
        }
        else if( it.key() == "Triggers" ) {
            if( !it.value().isArray() || comp.triggers().count() != 0 ) {
                if( error != nullptr ) *error = "\"triggers\" section must be specified as a single array";
                comp.clear();
                return comp;
            }
            QJsonArray array = it.value().toArray();
            for( auto it2 =array.begin(); it2 != array.end(); it2++ ) {
                Trigger t =Trigger::fromJson( (*it2).toObject(), error );
                if( !t.isValid() ) {
                    comp.clear();
                    return comp;
                }
                comp._triggers.append( t );
            }
        }
        else if( it.key() == "Name" ) {
            comp.setName( it.value().toString( comp.name() /*default*/ ) );
        }
    }

    return comp;
}

QByteArray 
Composition::toJson() const {

}

void
Composition::clear() {
    _tracks.clear();
    _triggers.clear();
}

const Trigger* 
Composition::triggerById( int id ) const {
    for( const auto &trig : _triggers )
        if( trig.id() == id ) return &trig;

    return nullptr;
}

const Track* 
Composition::trackById( int id ) const {
    for( const auto &track : _tracks )
        if( track.id() == id ) return &track;

    return nullptr;
}

bool 
Composition::isValid() const {
    if( _triggers.isEmpty() || _tracks.isEmpty() ) return false;
    for( auto & t : _triggers ) if( !t.isValid() ) return false;
    for( auto & t : _tracks )   if( !t.isValid() ) return false;
    return true;
}

