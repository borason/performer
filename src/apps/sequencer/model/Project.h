#pragma once

#include "Config.h"
#include "Observable.h"
#include "Types.h"
#include "ClockSetup.h"
#include "Track.h"
#include "Song.h"
#include "PlayState.h"
#include "UserScale.h"
#include "Routing.h"
#include "Serialize.h"
#include "FileDefs.h"

#include "core/math/Math.h"
#include "core/utils/StringBuilder.h"
#include "core/utils/StringUtils.h"

class Project {
public:
    static constexpr uint32_t Version = 2;

    //----------------------------------------
    // Types
    //----------------------------------------

    static constexpr size_t NameLength = FileHeader::NameLength;

    typedef std::array<Track, CONFIG_TRACK_COUNT> TrackArray;
    typedef std::array<uint8_t, CONFIG_CHANNEL_COUNT> CvOutputTrackArray;
    typedef std::array<uint8_t, CONFIG_CHANNEL_COUNT> GateOutputArray;

    Project();

    //----------------------------------------
    // Properties
    //----------------------------------------

    // slot

    int slot() const { return _slot; }
    void setSlot(int slot) {
        _slot = slot;
    }
    bool slotAssigned() const {
        return _slot != uint8_t(-1);
    }

    // name

    const char *name() const { return _name; }
    void setName(const char *name) {
        StringUtils::copy(_name, name, sizeof(_name));
    }

    // tempo

    float tempo() const { return _tempo; }
    void setTempo(float tempo) {
        _tempo = clamp(tempo, 1.f, 1000.f);
    }

    void editTempo(int value, bool shift) {
        setTempo(tempo() + value * (shift ? 0.1f : 1.f));
    }

    void printTempo(StringBuilder &str) const {
        str("%.1f", tempo());
    }

    // swing

    int swing() const { return _swing; }
    void setSwing(int swing) {
        _swing = clamp(swing, 50, 75);
    }

    void editSwing(int value, bool shift) {
        setSwing(ModelUtils::adjustedByStep(swing(), value, 5, !shift));
    }

    void printSwing(StringBuilder &str) const {
        str("%d%%", swing());
    }

    // syncMeasure

    int syncMeasure() const { return _syncMeasure; }
    void setSyncMeasure(int syncMeasure) {
        _syncMeasure = clamp(syncMeasure, 1, 128);
    }

    void editSyncMeasure(int value, bool shift) {
        setSyncMeasure(ModelUtils::adjustedByPowerOfTwo(syncMeasure(), value, shift));
    }

    void printSyncMeasure(StringBuilder &str) const {
        str("%d", syncMeasure());
    }

    // scale

    int scale() const { return _scale; }
    void setScale(int scale) {
        _scale = clamp(scale, 0, Scale::Count - 1);
        NoteSequence::_defaultScale = _scale;
    }

    void editScale(int value, bool shift) {
        setScale(scale() + value);
    }

    void printScale(StringBuilder &str) const {
        str(Scale::name(scale()));
    }

    const Scale &selectedScale() const {
        return Scale::get(scale());
    }

    // rootNote

    int rootNote() const { return _rootNote; }
    void setRootNote(int rootNote) {
        _rootNote = clamp(rootNote, 0, 11);
        NoteSequence::_defaultRootNote = _rootNote;
    }

    void editRootNote(int value, bool shift) {
        setRootNote(rootNote() + value);
    }

    void printRootNote(StringBuilder &str) const {
        Types::printNote(str, _rootNote);
    }

    // recordMode

    Types::RecordMode recordMode() const { return _recordMode; }
    void setRecordMode(Types::RecordMode recordMode) {
        _recordMode = ModelUtils::clampedEnum(recordMode);
    }

    void editRecordMode(int value, bool shift) {
        _recordMode = ModelUtils::adjustedEnum(_recordMode, value);
    }

    void printRecordMode(StringBuilder &str) const {
        str(Types::recordModeName(_recordMode));
    }

    // clockSetup

    const ClockSetup &clockSetup() const { return _clockSetup; }
          ClockSetup &clockSetup()       { return _clockSetup; }

    // tracks

    const TrackArray &tracks() const { return _tracks; }
          TrackArray &tracks()       { return _tracks; }

    const Track &track(int index) const { return _tracks[index]; }
          Track &track(int index)       { return _tracks[index]; }

    // cvOutputTrack

    const CvOutputTrackArray &cvOutputTracks() const { return _cvOutputTracks; }
          CvOutputTrackArray &cvOutputTracks()       { return _cvOutputTracks; }

    int cvOutputTrack(int index) const { return _cvOutputTracks[index]; }
    void setCvOutputTrack(int index, int trackIndex) { _cvOutputTracks[index] = clamp(trackIndex, 0, CONFIG_TRACK_COUNT - 1); }

    void editCvOutputTrack(int index, int value, bool shift) {
        setCvOutputTrack(index, cvOutputTrack(index) + value);
    }

    // gateOutputTrack

    const GateOutputArray &gateOutputTracks() const { return _gateOutputTracks; }
          GateOutputArray &gateOutputTracks()       { return _gateOutputTracks; }

    int gateOutputTrack(int index) const { return _gateOutputTracks[index]; }
    void setGateOutputTrack(int index, int trackIndex) { _gateOutputTracks[index] = clamp(trackIndex, 0, CONFIG_TRACK_COUNT - 1); }

    void editGateOutputTrack(int index, int value, bool shift) {
        setGateOutputTrack(index, gateOutputTrack(index) + value);
    }

    // song

    const Song &song() const { return _song; }
          Song &song()       { return _song; }

    // playState

    const PlayState &playState() const { return _playState; }
          PlayState &playState()       { return _playState; }

    // userScales

    const UserScale::Array &userScales() const { return UserScale::userScales; }
          UserScale::Array &userScales()       { return UserScale::userScales; }

    const UserScale &userScale(int index) const { return UserScale::userScales[index]; }
          UserScale &userScale(int index)       { return UserScale::userScales[index]; }

    // routing

    const Routing &routing() const { return _routing; }
          Routing &routing()       { return _routing; }

    // selectedTrackIndex

    int selectedTrackIndex() const { return _selectedTrackIndex; }
    void setSelectedTrackIndex(int index) {
        _selectedTrackIndex = clamp(index, 0, CONFIG_TRACK_COUNT - 1);
        _observable.notify(SelectedTrackIndex);
    }

    bool isSelectedTrack(int index) const { return _selectedTrackIndex == index; }

    // selectedPatternIndex

    int selectedPatternIndex() const {
        return _playState.snapshotActive() ? PlayState::SnapshotPatternIndex : _selectedPatternIndex;
    }

    void setSelectedPatternIndex(int index) {
        _selectedPatternIndex = clamp(index, 0, CONFIG_PATTERN_COUNT - 1);
        _observable.notify(SelectedPatternIndex);
    }

    bool isSelectedPattern(int index) const { return _selectedPatternIndex == index; }

    void editSelectedPatternIndex(int value, bool shift) {
        setSelectedPatternIndex(selectedPatternIndex() + value);
    }

    // selectedNoteSequenceLayer

    NoteSequence::Layer selectedNoteSequenceLayer() const { return _selectedNoteSequenceLayer; }
    void setSelectedNoteSequenceLayer(NoteSequence::Layer layer) { _selectedNoteSequenceLayer = layer; }

    // selectedCurveSequenceLayer

    CurveSequence::Layer selectedCurveSequenceLayer() const { return _selectedCurveSequenceLayer; }
    void setSelectedCurveSequenceLayer(CurveSequence::Layer layer) { _selectedCurveSequenceLayer = layer; }

    //----------------------------------------
    // Observable
    //----------------------------------------

    enum Property {
        SelectedTrackIndex,
        SelectedPatternIndex,
    };

    void watch(std::function<void(Property)> handler) {
        _observable.watch(handler);
    }

    //----------------------------------------
    // Methods
    //----------------------------------------

    void setTrackMode(int trackIndex, Track::TrackMode trackMode);

    const Track &selectedTrack() const { return _tracks[_selectedTrackIndex]; }
          Track &selectedTrack()       { return _tracks[_selectedTrackIndex]; }

    const NoteSequence &noteSequence(int trackIndex, int patternIndex) const { return _tracks[trackIndex].noteTrack().sequence(patternIndex); }
          NoteSequence &noteSequence(int trackIndex, int patternIndex)       { return _tracks[trackIndex].noteTrack().sequence(patternIndex); }

    const NoteSequence &selectedNoteSequence() const { return noteSequence(_selectedTrackIndex, _selectedPatternIndex); }
          NoteSequence &selectedNoteSequence()       { return noteSequence(_selectedTrackIndex, _selectedPatternIndex); }

    const CurveSequence &curveSequence(int trackIndex, int patternIndex) const { return _tracks[trackIndex].curveTrack().sequence(patternIndex); }
          CurveSequence &curveSequence(int trackIndex, int patternIndex)       { return _tracks[trackIndex].curveTrack().sequence(patternIndex); }

    const CurveSequence &selectedCurveSequence() const { return curveSequence(_selectedTrackIndex, _selectedPatternIndex); }
          CurveSequence &selectedCurveSequence()       { return curveSequence(_selectedTrackIndex, _selectedPatternIndex); }

    void clear();
    void clearPattern(int patternIndex);

    void demoProject();

    void write(WriteContext &context) const;
    bool read(ReadContext &context);

    fs::Error write(const char *path) const;
    fs::Error read(const char *path);

private:
    uint8_t _slot = uint8_t(-1);
    char _name[NameLength + 1];
    float _tempo;
    uint8_t _swing;
    uint8_t _syncMeasure;
    uint8_t _scale;
    uint8_t _rootNote;
    Types::RecordMode _recordMode;

    ClockSetup _clockSetup;
    TrackArray _tracks;
    CvOutputTrackArray _cvOutputTracks;
    GateOutputArray _gateOutputTracks;
    Song _song;
    PlayState _playState;
    Routing _routing;

    int _selectedTrackIndex = 0;
    int _selectedPatternIndex = 0;
    NoteSequence::Layer _selectedNoteSequenceLayer = NoteSequence::Layer(0);
    CurveSequence::Layer _selectedCurveSequenceLayer = CurveSequence::Layer(0);

    Observable<Property, 2> _observable;
};
