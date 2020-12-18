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

Trigger::Trigger() : _id( -1 ) { }
Trigger::~Trigger() { }

Trigger
Trigger::fromJson( const QJsonObject& json, QString* error ) {
    Trigger t;
    for( auto it =json.begin(); it != json.end(); it++ ) {
	if( it.key().toLower() == "id" )
	    t.setId( it.value().toInt( t.id()/*default*/ ) );
        else if( it.key().toLower() == "events" && it.value().isArray() ) {
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
    
bool 
Trigger::addEventFromJson( const QJsonObject& json, QString* error ) {
    Event e;
    // Set some default, TODO: move
    e.midiEvent.setVoice( 0 );
    e.midiEvent.setValue( 0 );
    e.midiEvent.setNumber( 0 );
    e.midiEvent.setNote( 60 );
    e.midiEvent.setVelocity( 60 );

    for( auto it =json.begin(); it != json.end(); it++ ) {
        if( it.key().toLower() == "delay" ) {
            e.delay = it.value().toInt( 0 );
        }
        else if( it.key().toLower() == "type" )
            e.midiEvent.setType( eventTypeFromStr( it.value().toString( "" ) ) );
        else if( it.key().toLower() == "note" )
            e.midiEvent.setNote( it.value().toInt() );
        else if( it.key().toLower() == "voice" )
            e.midiEvent.setVoice( it.value().toInt() );
        else if( it.key().toLower() == "velocity" )
            e.midiEvent.setVelocity( it.value().toInt() );
        else if( it.key().toLower() == "value" )
            e.midiEvent.setValue( it.value().toInt() );
        else if( it.key().toLower() == "number" )
            e.midiEvent.setNumber( it.value().toInt() );
    }
    if( e.midiEvent.type() == QMidiEvent::Invalid ) goto ERROR;
    _events.append( e );
    return true;
ERROR:
    if( error != nullptr ) *error = "Incompletely specified trigger/MIDI event";
    return false;

}

/* Class Track implementation */

Track::Track()
    : _tempo( 0.0 ), _length( 360 ) { }
Track::~Track() { }

Track 
Track::fromJson( const QJsonObject& json, QString* error ) {
    Track t;
    for( auto it =json.begin(); it != json.end(); it++ ) {
        if( it.key().toLower() == "train" && it.value().isObject() ) {
            QJsonObject obj =it.value().toObject();
            for( auto it2 =obj.begin(); it2 != obj.end(); it2++ ) {
                if( it2.key().toLower() == "tempo" )
                    t.setTempo( it2.value().toDouble( 0.0 ) );
                else if( it2.key().toLower() == "length" )
                    t.setLength( it2.value().toInt( 360 ) );
                else if( it2.key().toLower() == "axleoffsets" )
                    if( !t.setOffsetsFromJson( it2.value().toArray(), error ) )
                        return t;
            }
        } else if( it.key().toLower() == "sections" && it.value().isArray() ) {
            QJsonArray array = it.value().toArray();
            for( auto it2 =array.begin(); it2 != array.end(); it2++ ) {
                if( !t.addSectionFromJson( (*it2).toObject(), error ) )
                    return t;
            }
        }
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
    bool haveOffset =false;
    for( auto it =json.begin(); it != json.end(); it++ ) {
        if( it.key().toLower() == "offset" ) {
            if( !haveOffset ) haveOffset =true;
            else goto ERROR;
            s.offset = it.value().toDouble( 0.0 );
        }
        else if( it.key().toLower() == "onenter" )
            s.on_enter = it.value().toInt( );
        else if( it.key().toLower() == "onleave" )
            s.on_leave = it.value().toInt( );
    }
    if( !haveOffset ) goto ERROR;
    _sections.append( s );
    return true;
ERROR:
    if( error != nullptr ) *error = "Incompletely specified section";
    return false;
}


/* Class Composition implementation */



Composition::Composition() : _nextIndex(0) { }
Composition::~Composition() { }

Composition 
Composition::fromJson( const QByteArray& json, QString* error ) {
    Composition comp;
    QJsonParseError err;
    QJsonDocument doc =QJsonDocument::fromJson( json, &err );

    if( err.error != QJsonParseError::NoError ) {
        if( error != nullptr )
            *error = err.errorString();
        return comp;
    }

    if( !doc.isObject() || doc.isEmpty() ) {
        if( error != nullptr ) *error = "Expected a root-level object in the document";
        return comp;
    }

    QJsonObject root =doc.object();

    for( auto it =root.begin(); it != root.end(); it++ ) {
        //printf( "Key: %s\n", qPrintable(it.key()) );
        if( it.key().toLower() == "tracks" ) {
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
                t.setIndex( comp._nextIndex++ );
                comp._tracks.append( t );
            }
        }
        else if( it.key().toLower() == "triggers" ) {
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
        else if( it.key().toLower() == "name" ) {
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

bool 
Composition::isValid() const {
    if( _triggers.isEmpty() || _tracks.isEmpty() ) return false;
    for( auto & t : _triggers ) if( !t.isValid() ) return false;
    for( auto & t : _tracks )   if( !t.isValid() ) return false;
    return true;
}

