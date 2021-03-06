/*
 * MidiTrain -- MIDI sequencer and visualizer based on a train-inspired musical notation
 *
 * Author: Micky Faas <micky@edukitty.org>
 * This work is released under the MIT license
 */

#pragma once

#include <QObject>
#include <QVector>
#include <QMidiFile.h>
#include <QByteArray>
#include <QString>

class Trigger {
public:
    enum EventType {
        NoEvent,
        MidiEvent,
        StopEvent,
        StartEvent,
        ResetEvent
    };
    struct Event {
        EventType type;
        int midiDelay, midiDuration;
        QMidiEvent midiEvent;
        int target;
    };

    typedef QVector<Event> EventVectorT;

    Trigger();
    ~Trigger();

    void addEvent( const Event& );

    const EventVectorT& events() const { return _events; }
    //EventVectorT& events() { return _events; }

    static Trigger fromJson( const QJsonObject&, QString* error =nullptr );
    
    inline void setId( int i ) { _id = i; }
    inline int id() const { return _id; }

    bool isValid() const;

    inline bool hasStopEvent() const { return _hasStop; }
    void setHasStop( bool b ) { _hasStop =b; }

private:
    bool addEventFromJson( const QJsonObject&, QString* error );
    EventVectorT _events;
    int _id;
    bool _hasStop;
    
};

class Track {
public:
    struct Section {
        double offset;
        int trigger;
        int transpose;
    };

    typedef QVector<Section> SectionVectorT;
    typedef QVector<double> OffsetVectorT;

    Track();
    ~Track();
    
    static Track fromJson( const QJsonObject&, QString* error =nullptr );

    inline bool autoStart() const { return _start; }
    inline void setAutoStart( bool b ) { _start =b; }

    inline int midiChannel() const { return _channel; }
    inline void setMidiChannel( int i ) { _channel =i; }

    const SectionVectorT& sections() const { return _sections; }
    //SectionVectorT& sections() { return _sections; }

    const OffsetVectorT& axleOffsets() const { return _offsets; }
    //OffsetVectorT& axleOffsets(){ return _offsets; }

    int axleCount() const { return _offsets.count() + 1; }

    void setLength( int i = 360 ) { _length =i; }
    int length() const { return _length; }

    void setTempo( double d ) { _tempo =d; }
    double tempo() const { return _tempo; }

    void setId( int i ) { _id =i; }
    int id() const { return _id; }

    void setLoopCount( int i ) { _loop =i; }
    int loopCount() const { return _loop; }

    bool isValid() const;
    
private:
    bool setOffsetsFromJson( const QJsonArray& array, QString *error );
    bool addSectionFromJson( const QJsonObject& json, QString *error );
    SectionVectorT _sections;
    OffsetVectorT _offsets;
    double _tempo;
    int _length;
    int _id;
    bool _start;
    int _channel;
    int _loop;
};

class Composition {
public:
    typedef QVector<Track>   TrackVectorT;
    typedef QVector<Trigger> TriggerVectorT;

    Composition();
    ~Composition();

    static Composition fromJson( const QByteArray&, QString* error =nullptr );
    QByteArray toJson() const;

    void setName( const QString& s ) { _name =s; }
    QString name() const { return _name; }

    void clear();
    bool isValid() const;

    const TrackVectorT& tracks() const { return _tracks; }
    //TrackVectorT& tracks() { return _tracks; }

    const TriggerVectorT& triggers() const { return _triggers; }
    //TriggerVectorT& triggers() { return _triggers; }
    //
    const Track* trackById( int id ) const;
    
    const Trigger* triggerById( int id ) const;

private:

    TrackVectorT _tracks;
    TriggerVectorT _triggers;
    QString _name;
    int _maxId;

};

