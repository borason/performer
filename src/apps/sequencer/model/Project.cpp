#include "Project.h"

#include "core/fs/FileWriter.h"
#include "core/fs/FileReader.h"

Project::Project() :
    _playState(*this),
    _routing(*this)
{
    for (size_t i = 0; i < _tracks.size(); ++i) {
        _tracks[i].setTrackIndex(i);
    }

    clear();
}

void Project::clear() {
    _slot = uint8_t(-1);
    StringUtils::copy(_name, "INIT", sizeof(_name));
    setTempo(120.f);
    setSwing(50);
    setSyncMeasure(1);
    setScale(0);
    setRootNote(0);

    _clockSetup.clear();

    for (auto &track : _tracks) {
        track.clear();
    }

    for (int i = 0; i < CONFIG_CHANNEL_COUNT; ++i) {
        _cvOutputTracks[i] = i;
        _gateOutputTracks[i] = i;
    }

    _song.clear();
    _playState.clear();
    _routing.clear();

    setSelectedTrackIndex(0);
    setSelectedPatternIndex(0);

    // TODO remove
    demoProject();
}

void Project::clearPattern(int patternIndex) {
    for (auto &track : _tracks) {
        track.clearPattern(patternIndex);
    }
}

void Project::demoProject() {
#if 1
    noteSequence(0, 0).setLastStep(15);
    noteSequence(0, 0).setGates({ 1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0 });
    noteSequence(1, 0).setLastStep(15);
    noteSequence(1, 0).setGates({ 0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0 });
    noteSequence(2, 0).setLastStep(15);
    noteSequence(2, 0).setGates({ 0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0 });
    noteSequence(3, 0).setLastStep(15);
    noteSequence(3, 0).setGates({ 0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0 });
    noteSequence(4, 0).setLastStep(15);
    noteSequence(4, 0).setGates({ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 });
    noteSequence(5, 0).setLastStep(15);
    noteSequence(5, 0).setGates({ 0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0 });
    noteSequence(7, 0).setLastStep(15);
    noteSequence(7, 0).setGates({ 1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1 });
    noteSequence(7, 0).setNotes({ 0,0,0,0,12,0,12,1,24,21,22,0,3,6,12,1 });
#endif
}

void Project::setTrackMode(int trackIndex, Track::TrackMode trackMode) {
    // TODO make sure engine is synced to this before updating UI
    // TODO reset snapshots
    _tracks[trackIndex].setTrackMode(trackMode);
}

void Project::write(WriteContext &context) const {
    auto &writer = context.writer;
    writer.write(_tempo);
    writer.write(_swing);
    writer.write(_syncMeasure);
    writer.write(_scale);
    writer.write(_rootNote);

    _clockSetup.write(context);

    writeArray(context, _tracks);
    writeArray(context, _cvOutputTracks);
    writeArray(context, _gateOutputTracks);

    _song.write(context);
    _playState.write(context);
    _routing.write(context);

    writer.write(_selectedTrackIndex);
    writer.write(_selectedPatternIndex);

    writer.writeHash();
}

bool Project::read(ReadContext &context) {
    clear();

    auto &reader = context.reader;
    reader.read(_tempo);
    reader.read(_swing);
    reader.read(_syncMeasure);
    reader.read(_scale);
    reader.read(_rootNote);

    _clockSetup.read(context);

    readArray(context, _tracks);
    readArray(context, _cvOutputTracks);
    readArray(context, _gateOutputTracks);

    _song.read(context);
    _playState.read(context);
    _routing.read(context);

    reader.read(_selectedTrackIndex);
    reader.read(_selectedPatternIndex);

    bool success = reader.checkHash();
    if (!success) {
        clear();
    }

    return success;
}

fs::Error Project::write(const char *path) const {
    fs::FileWriter fileWriter(path);
    if (fileWriter.error() != fs::OK) {
        fileWriter.error();
    }

    FileHeader header(FileType::Project, 0, _name);
    fileWriter.write(&header, sizeof(header));

    VersionedSerializedWriter writer(
        [&fileWriter] (const void *data, size_t len) { fileWriter.write(data, len); },
        Version
    );

    WriteContext context = { writer };
    write(context);

    return fileWriter.finish();
}

fs::Error Project::read(const char *path) {
    fs::FileReader fileReader(path);
    if (fileReader.error() != fs::OK) {
        fileReader.error();
    }

    FileHeader header;
    fileReader.read(&header, sizeof(header));
    header.readName(_name, sizeof(_name));

    VersionedSerializedReader reader(
        [&fileReader] (void *data, size_t len) { fileReader.read(data, len); },
        Version
    );

    ReadContext context = { reader };
    bool success = read(context);

    auto error = fileReader.finish();
    if (error == fs::OK && !success) {
        error = fs::INVALID_CHECKSUM;
    }

    return error;
}
