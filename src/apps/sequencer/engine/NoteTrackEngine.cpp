#include "NoteTrackEngine.h"

#include "Groove.h"
#include "SequenceUtils.h"

#include "core/Debug.h"
#include "core/utils/Random.h"
#include "core/math/Math.h"

#include "model/Scale.h"

static Random rng;

// evaluate if step gate is active
static bool evalStepGate(const NoteSequence::Step &step, int gateProbabilityOffset) {
    int gateProbability = clamp(step.gateProbability() + gateProbabilityOffset, -1, NoteSequence::GateProbability::Max);
    return step.gate() &&
        (gateProbability == NoteSequence::GateProbability::Max ||
         int(rng.nextRange(NoteSequence::GateProbability::Range)) <= gateProbability);
}

// evaluate step retrigger count
static int evalStepRetrigger(const NoteSequence::Step &step) {
    return
        (step.retriggerProbability() == NoteSequence::RetriggerProbability::Max ||
         int(rng.nextRange(NoteSequence::RetriggerProbability::Range)) <= step.retriggerProbability()) ?
         step.retrigger() + 1 : 1;
}

// evaluate step length
static int evalStepLength(const NoteSequence::Step &step, int lengthOffset) {
    int length = NoteSequence::Length::clamp(step.length() + lengthOffset) + 1;
    if (step.lengthVariationProbability() > 0) {
        if (int(rng.nextRange(NoteSequence::LengthVariationProbability::Range)) < step.lengthVariationProbability()) {
            int offset = step.lengthVariationRange() == 0 ? 0 : rng.nextRange(std::abs(step.lengthVariationRange()) + 1);
            if (step.lengthVariationRange() < 0) {
                offset = -offset;
            }
            length = clamp(length + offset, 0, NoteSequence::Length::Range);
        }
    }
    return length;
}

// evaluate note voltage
static float evalStepNote(const NoteSequence::Step &step, const Scale &scale, int rootNote, int octave, int transpose, bool useVariation = true) {
    int note = step.note() + (scale.isChromatic() ? rootNote : 0) + octave * scale.notesPerOctave() + transpose;
    if (step.noteVariationProbability() > 0) {
        if (useVariation && int(rng.nextRange(NoteSequence::NoteVariationProbability::Range)) < step.noteVariationProbability()) {
            int offset = step.noteVariationRange() == 0 ? 0 : rng.nextRange(std::abs(step.noteVariationRange()) + 1);
            if (step.noteVariationRange() < 0) {
                offset = -offset;
            }
            note = NoteSequence::Note::clamp(note + offset);
        }
    }
    return scale.noteVolts(note);
}

void NoteTrackEngine::reset() {
    _sequenceState.reset();
    _currentStep = -1;
    _activity = false;
    _gateOutput =false;
    _cvOutput = 0.f;
    _cvOutputTarget = 0.f;
    _slideActive = false;
    _gateQueue.clear();
    _cvQueue.clear();

    changePattern();
}

void NoteTrackEngine::tick(uint32_t tick) {
    ASSERT(_sequence != nullptr, "invalid sequence");
    const auto &sequence = *_sequence;
    const auto *linkData = _linkedTrackEngine ? _linkedTrackEngine->linkData() : nullptr;

    if (linkData) {
        _linkData = *linkData;
        _sequenceState = *linkData->sequenceState;

        if (linkData->relativeTick == 0) {
            recordStep(tick, linkData->divisor);
            triggerStep(tick, linkData->divisor);
        }
    } else {
        uint32_t divisor = sequence.divisor() * (CONFIG_PPQN / CONFIG_SEQUENCE_PPQN);
        uint32_t measureDivisor = (sequence.resetMeasure() * CONFIG_PPQN * 4);
        uint32_t relativeTick = measureDivisor == 0 ? tick : tick % measureDivisor;

        // handle reset measure
        if (relativeTick == 0) {
            reset();
        }

        relativeTick %= divisor;

        // advance sequence
        if (relativeTick == 0) {
            switch (_noteTrack.playMode()) {
            case Types::PlayMode::Aligned:
                _sequenceState.advanceAligned(tick / divisor, sequence.runMode(), sequence.firstStep(), sequence.lastStep(), rng);
                break;
            case Types::PlayMode::Free:
                _sequenceState.advanceFree(sequence.runMode(), sequence.firstStep(), sequence.lastStep(), rng);
                break;
            case Types::PlayMode::Last:
                break;
            }

            recordStep(tick, divisor);
            triggerStep(tick, divisor);
        }

        _linkData.divisor = divisor;
        _linkData.relativeTick = relativeTick;
        _linkData.sequenceState = &_sequenceState;
    }

    while (!_gateQueue.empty() && tick >= _gateQueue.front().tick) {
        _activity = _gateQueue.front().gate;
        _gateOutput = (!mute() || fill()) && _activity;
        _gateQueue.pop();
    }

    while (!_cvQueue.empty() && tick >= _cvQueue.front().tick) {
        _cvOutputTarget = _cvQueue.front().cv;
        _slideActive = _cvQueue.front().slide;
        _cvQueue.pop();
    }
}

void NoteTrackEngine::update(float dt) {
    bool running = _engineState.running();
    bool recording = _engineState.recording();

    const auto &sequence = *_sequence;
    const auto &scale = sequence.selectedScale();
    int rootNote = sequence.selectedRootNote();

    // override due to monitoring or recording
    if (!running || recording) {
        if (!recording && _monitorStepIndex >= 0) {
            // step monitoring (first priority)
            const auto &step = sequence.step(_monitorStepIndex);
            int octave = _noteTrack.octave();
            int transpose = _noteTrack.transpose();
            _cvOutputTarget = evalStepNote(step, scale, rootNote, octave, transpose, false);
            _activity = _gateOutput = true;
            _monitorOverrideActive = true;
        } else if (_recordHistory.isNoteActive()) {
            // midi monitoring (second priority)
            if (scale.isChromatic()) {
                int note = scale.noteFromVolts((_recordHistory.activeNote() - 60 - rootNote) * (1.f / 12.f));
                _cvOutputTarget = scale.noteVolts(note) + (rootNote / 12.f);
            } else {
                int note = scale.noteFromVolts((_recordHistory.activeNote() - 60) * (1.f / 12.f));
                _cvOutputTarget = scale.noteVolts(note);
            }
            _activity = _gateOutput = true;
            _monitorOverrideActive = true;
        } else {
            if (_monitorOverrideActive) {
                _activity = _gateOutput = false;
                _monitorOverrideActive = false;
            }
        }
    }

    if (_slideActive && _noteTrack.slideTime() > 0) {
        _cvOutput += (_cvOutputTarget - _cvOutput) * std::min(1.f, dt * (200 - 2 * _noteTrack.slideTime()));
    } else {
        _cvOutput = _cvOutputTarget;
    }
}

void NoteTrackEngine::changePattern() {
    _sequence = &_noteTrack.sequence(pattern());
    _fillSequence = &_noteTrack.sequence(std::min(pattern() + 1, CONFIG_PATTERN_COUNT - 1));
}

void NoteTrackEngine::monitorMidi(uint32_t tick, const MidiMessage &message) {
    _recordHistory.write(tick, message);
}

void NoteTrackEngine::setMonitorStep(int index) {
    _monitorStepIndex = (index >= 0 && index < CONFIG_STEP_COUNT) ? index : -1;
}

void NoteTrackEngine::triggerStep(uint32_t tick, uint32_t divisor) {
    int octave = _noteTrack.octave();
    int transpose = _noteTrack.transpose();
    int rotate = _noteTrack.rotate();
    bool useFillGates = fill() && _noteTrack.fillMode() == Types::FillMode::Gates;
    bool useFillSequence = fill() && _noteTrack.fillMode() == Types::FillMode::NextPattern;

    const auto &sequence = *_sequence;
    const auto &evalSequence = useFillSequence ? *_fillSequence : *_sequence;
    _currentStep = SequenceUtils::rotateStep(_sequenceState.step(), sequence.firstStep(), sequence.lastStep(), rotate);
    const auto &step = evalSequence.step(_currentStep);

    if (evalStepGate(step, _noteTrack.stepProbabilityBias()) || useFillGates) {
        uint32_t stepLength = (divisor * evalStepLength(step, _noteTrack.stepLengthBias())) / NoteSequence::Length::Range;
        int stepRetrigger = evalStepRetrigger(step);
        if (stepRetrigger > 1) {
            uint32_t retriggerLength = divisor / stepRetrigger;
            uint32_t stepOffset = 0;
            while (stepRetrigger-- > 0 && stepOffset <= stepLength) {
                _gateQueue.push({ applySwing(tick + stepOffset), true });
                _gateQueue.push({ applySwing(tick + stepOffset + retriggerLength / 2), false });
                stepOffset += retriggerLength;
            }
        } else {
            _gateQueue.push({ applySwing(tick), true });
            _gateQueue.push({ applySwing(tick + stepLength), false });
        }

        const auto &scale = evalSequence.selectedScale();
        int rootNote = evalSequence.selectedRootNote();
        _cvQueue.push({ applySwing(tick), evalStepNote(step, scale, rootNote, octave, transpose), step.slide() });
    }
}

void NoteTrackEngine::recordStep(uint32_t tick, uint32_t divisor) {
    if (!_engineState.recording() || _sequenceState.lastStep() < 0) {
        return;
    }

    uint32_t startTick = 0;
    uint32_t endTick = 0;
    int note = -1;

    for (size_t i = 0; i < _recordHistory.size(); ++i) {
        const auto &event = _recordHistory[i];
        if (event.tick >= tick) {
            break;
        }

        switch (event.type) {
        case RecordHistory::Type::NoteOn:
            startTick = endTick = event.tick;
            note = event.note;
            break;
        case RecordHistory::Type::NoteOff:
            if (event.tick + divisor < tick) {
                note = -1;
            }
            endTick = event.tick;
            break;
        }
    }

    if (note == -1) {
        return;
    }

    // if (tick - startTick > divisor) {
    //     return;
    // }

    int length = startTick == endTick ?
        NoteSequence::Length::max() :
        std::min(NoteSequence::Length::max(), (int(endTick - startTick) * NoteSequence::Length::range()) / int(divisor));

    DBG("%d, %d, %d", _sequenceState.lastStep(), note, length);

    auto &sequence = *_sequence;
    auto &step = sequence.step(_sequenceState.lastStep());

    const auto &scale = sequence.selectedScale();
    int rootNote = sequence.selectedRootNote();

    if (scale.isChromatic()) {
        note = scale.noteFromVolts((note - 60 - rootNote) * (1.f / 12.f));
    } else {
        note = scale.noteFromVolts((note - 60) * (1.f / 12.f));
    }

    step.setGate(true);
    step.setGateProbability(NoteSequence::GateProbability::Max);
    step.setRetrigger(0);
    step.setRetriggerProbability(0);
    step.setLength(length);
    step.setLengthVariationRange(0);
    step.setLengthVariationProbability(0);
    step.setNote(note);
    step.setNoteVariationRange(0);
    step.setNoteVariationProbability(0);
}

uint32_t NoteTrackEngine::applySwing(uint32_t tick) {
    return Groove::swing(tick, CONFIG_PPQN / 4, swing());
}
