#pragma once

#include "Config.h"

#include "Types.h"
#include "MidiConfig.h"
#include "Serialize.h"
#include "ModelUtils.h"

#include "core/math/Math.h"
#include "core/utils/StringBuilder.h"

#include <array>

#include <cstdint>

class Project;
class NoteSequence;
class CurveSequence;

class Routing {
public:
    //----------------------------------------
    // Types
    //----------------------------------------

    enum class Target : uint8_t {
        None,
        // Global targets
        Tempo,
        GlobalFirst = Tempo,
        Swing,
        GlobalLast = Swing,

        // Global modulations
        // GlobalModFirst,
        // CvOffset = GlobalModFirst,
        // GateProbabilityOffset,
        // GateLengthOffset,
        // GateLengthProbabilityOffset,
        // RetriggerOffset,
        // RetriggerProbabilityOffset,
        // SequenceShift,
        // GlobalModLast = SequenceShift,

        // Track targets
        TrackSlideTime,
        TrackFirst = TrackSlideTime,
        TrackOctave,
        TrackTranspose,
        TrackRotate,
        TrackStepGateProbabilityBias,
        TrackStepLengthBias,
        TrackLast = TrackStepLengthBias,

        // Sequence targets
        FirstStep,
        SequenceFirst = FirstStep,
        LastStep,
        SequenceLast = LastStep,

        Last,
    };

    static const char *targetName(Target target) {
        switch (target) {
        case Target::None:              return "None";
        case Target::Tempo:             return "Tempo";
        case Target::Swing:             return "Swing";

        case Target::TrackSlideTime:    return "Slide Time";
        case Target::TrackOctave:       return "Octave";
        case Target::TrackTranspose:    return "Transpose";
        case Target::TrackRotate:       return "Rotate";
        case Target::TrackStepGateProbabilityBias: return "Gate Prob Bias";
        case Target::TrackStepLengthBias: return "Length Bias";

        case Target::FirstStep:         return "First Step";
        case Target::LastStep:          return "Last Step";

        case Target::Last:              break;
        }
        return nullptr;
    }

    static bool isTrackTarget(Target target) {
        return target >= Target::TrackFirst && target <= Target::TrackLast;
    }

    static bool isSequenceTarget(Target target) {
        return target >= Target::SequenceFirst && target <= Target::SequenceLast;
    }

    enum class Source : uint8_t {
        None,
        CvIn1,
        CvFirst = CvIn1,
        CvIn2,
        CvIn3,
        CvIn4,
        CvOut1,
        CvOut2,
        CvOut3,
        CvOut4,
        CvOut5,
        CvOut6,
        CvOut7,
        CvOut8,
        CvLast = CvOut8,
        Midi,
        Last
    };

    static bool isCvSource(Source source) { return source >= Source::CvFirst && source <= Source::CvLast; }
    static bool isMidiSource(Source source) { return source == Source::Midi; }

    static void printSource(Source source, StringBuilder &str) {
        switch (source) {
        case Source::None:
            str("None");
            break;
        case Source::CvIn1:
        case Source::CvIn2:
        case Source::CvIn3:
        case Source::CvIn4:
            str("CV In %d", int(source) - int(Source::CvIn1) + 1);
            break;
        case Source::CvOut1:
        case Source::CvOut2:
        case Source::CvOut3:
        case Source::CvOut4:
        case Source::CvOut5:
        case Source::CvOut6:
        case Source::CvOut7:
        case Source::CvOut8:
            str("CV Out %d", int(source) - int(Source::CvOut1) + 1);
            break;
        case Source::Midi:
            str("MIDI");
        case Source::Last:
            break;
        }
    }

    class CvSource {
    public:
        // range

        Types::VoltageRange range() const { return _range; }
        void setRange(Types::VoltageRange range) {
            _range = ModelUtils::clampedEnum(range);
        }

        void editRange(int value, bool shift) {
            setRange(ModelUtils::adjustedEnum(range(), value));
        }

        void printRange(StringBuilder &str) const {
            str(Types::voltageRangeName(range()));
        }

        void clear();

        void write(WriteContext &context) const;
        void read(ReadContext &context);

        bool operator==(const CvSource &other) const;

    private:
        Types::VoltageRange _range;
    };

    class MidiSource {
    public:
        enum class Event : uint8_t {
            ControlAbsolute,
            ControlRelative,
            LastControlEvent = ControlRelative,
            PitchBend,
            NoteMomentary,
            NoteToggle,
            NoteVelocity,
            Last,
        };

        static const char *eventName(Event event) {
            switch (event) {
            case Event::ControlAbsolute:return "CC Absolute";
            case Event::ControlRelative:return "CC Relative";
            case Event::PitchBend:      return "Pitch Bend";
            case Event::NoteMomentary:  return "Note Momentary";
            case Event::NoteToggle:     return "Note Toggle";
            case Event::NoteVelocity:   return "Note Velocity";
            case Event::Last:           break;
            }
            return nullptr;
        }

        // source

        const MidiSourceConfig &source() const { return _source; }
              MidiSourceConfig &source()       { return _source; }

        // event

        Event event() const { return _event; }
        void setEvent(Event event) {
            _event = ModelUtils::clampedEnum(event);
        }

        void editEvent(int value, bool shift) {
            setEvent(ModelUtils::adjustedEnum(event(), value));
        }

        void printEvent(StringBuilder &str) const {
            str(eventName(event()));
        }

        bool isControlEvent() const {
            return int(_event) <= int(Event::LastControlEvent);
        }

        // note

        int note() const { return _controlNumberOrNote; }
        void setNote(int note) {
            _controlNumberOrNote = clamp(note, 0, 127);
        }

        void editNote(int value, bool shift) {
            setNote(note() + value);
        }

        void printNote(StringBuilder &str) const {
            Types::printMidiNote(str, note());
        }

        // controlNumber

        int controlNumber() const { return _controlNumberOrNote; }
        void setControlNumber(int controlNumber) {
            _controlNumberOrNote = clamp(controlNumber, 0, 127);
        }

        void editControlNumber(int value, bool shift) {
            setControlNumber(controlNumber() + value);
        }

        void printControlNumber(StringBuilder &str) const {
            str("%d", note());
        }

        void clear();

        void write(WriteContext &context) const;
        void read(ReadContext &context);

        bool operator==(const MidiSource &other) const;

    private:
        MidiSourceConfig _source;
        Event _event;
        uint8_t _controlNumberOrNote;
    };

    class Route {
    public:
        // target

        Target target() const { return _target; }
        void setTarget(Target target) {
            _target = ModelUtils::clampedEnum(target);
        }

        void editTarget(int value, bool shift) {
            setTarget(ModelUtils::adjustedEnum(target(), value));
        }

        void printTarget(StringBuilder &str) const {
            str(targetName(target()));
        }

        // tracks

        uint8_t tracks() const { return _tracks; }
        void setTracks(uint8_t tracks) {
            _tracks = tracks;
        }

        void toggleTrack(int trackIndex) {
            if (tracks() & (1<<trackIndex)) {
                setTracks(tracks() & ~(1<<trackIndex));
            } else {
                setTracks(tracks() | (1<<trackIndex));
            }
        }

        void printTracks(StringBuilder &str) const {
            if (isTrackTarget(_target) || isSequenceTarget(_target)) {
                for (int i = 0; i < CONFIG_TRACK_COUNT; ++i) {
                    str("%c", (_tracks & (1<<i)) ? 'X' : '-');
                }
            } else {
                str("n/a");
            }
        }

        // min

        float min() const { return _min; }
        void setMin(float min) {
            _min = clamp(min, 0.f, 1.f);
            if (max() < _min) {
                setMax(_min);
            }
        }

        void editMin(int value, bool shift) {
            setMin(min() + value * targetValueStep(_target));
        }

        void printMin(StringBuilder &str) const {
            Routing::printTargetValue(_target, _min, str);
        }

        // max

        float max() const { return _max; }
        void setMax(float max) {
            _max = clamp(max, 0.f, 1.f);
            if (min() > _max) {
                setMin(_max);
            }
        }

        void editMax(int value, bool shift) {
            setMax(max() + value * targetValueStep(_target));
        }

        void printMax(StringBuilder &str) const {
            Routing::printTargetValue(_target, _max, str);
        }

        // source

        Source source() const { return _source; }
        void setSource(Source source) {
            _source = ModelUtils::clampedEnum(source);
        }

        void editSource(int value, bool shift) {
            setSource(ModelUtils::adjustedEnum(source(), value));
        }

        void printSource(StringBuilder &str) const {
            Routing::printSource(source(), str);
        }

        // cvSource

        const CvSource &cvSource() const { return _cvSource; }
              CvSource &cvSource()       { return _cvSource; }

        // midiSource

        const MidiSource &midiSource() const { return _midiSource; }
              MidiSource &midiSource()       { return _midiSource; }

        Route();

        void clear();

        bool active() const { return _target != Target::None; }

        void init(Target target, int track = -1);

        void write(WriteContext &context) const;
        void read(ReadContext &context);

        bool operator==(const Route &other) const;
        bool operator!=(const Route &other) const {
            return !(*this == other);
        }

    private:
        Target _target;
        int8_t _tracks;
        float _min; // TODO make these int16_t
        float _max;
        Source _source;
        CvSource _cvSource;
        MidiSource _midiSource;

        friend class Routing;
    };

    typedef std::array<Route, CONFIG_ROUTE_COUNT> RouteArray;

    //----------------------------------------
    // Properties
    //----------------------------------------

    // routes

    const RouteArray &routes() const { return _routes; }
          RouteArray &routes()       { return _routes; }

    const Route &route(int index) const { return _routes[index]; }
          Route &route(int index)       { return _routes[index]; }

    //----------------------------------------
    // Methods
    //----------------------------------------

    Routing(Project &project);

    void clear();

    int findEmptyRoute() const;
    int findRoute(Target target, int trackIndex) const;

    void writeTarget(Target target, int trackIndex, int patternIndex, float normalized);
    float readTarget(Target target, int trackIndex, int patternIndex) const;

    void write(WriteContext &context) const;
    void read(ReadContext &context);

    bool isDirty() const { return _dirty; }
    void clearDirty() { _dirty = false; }

private:
    void writeTarget(Target target, int trackIndex, int patternIndex, float floatValue, int intValue);
    void writeTrackTarget(Target target, int trackIndex, int patternIndex, float floatValue, int intValue);
    void writeNoteSequenceTarget(NoteSequence &sequence, Target target, float floatValue, int intValue);
    void writeCurveSequenceTarget(CurveSequence &sequence, Target target, float floatValue, int intValue);

    static float normalizeTargetValue(Target target, float value);
    static float denormalizeTargetValue(Target target, float normalized);
    static float targetValueStep(Target target);
    static void printTargetValue(Target target, float normalized, StringBuilder &str);

    Project &_project;
    RouteArray _routes;
    bool _dirty;
};
