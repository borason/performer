#include "RoutingEngine.h"

#include "Engine.h"

// for allowing direct mapping
static_assert(int(MidiPort::Midi) == int(Types::MidiPort::Midi), "invalid mapping");
static_assert(int(MidiPort::UsbMidi) == int(Types::MidiPort::UsbMidi), "invalid mapping");

RoutingEngine::RoutingEngine(Engine &engine, Model &model) :
    _engine(engine),
    _routing(model.project().routing())
{}

void RoutingEngine::update() {
    updateSources();
    updateSinks();
}

bool RoutingEngine::receiveMidi(MidiPort port, const MidiMessage &message) {
    bool consumed = false;

    for (int routeIndex = 0; routeIndex < CONFIG_ROUTE_COUNT; ++routeIndex) {
        const auto &route = _routing.route(routeIndex);
        if (route.active() &&
            route.source() == Routing::Source::Midi &&
            route.midiSource().source().port() == Types::MidiPort(port) &&
            (route.midiSource().source().channel() == 0 || route.midiSource().source().channel() == message.channel() + 1)
        ) {
            const auto &midiSource = route.midiSource();
            auto &sourceValue = _sourceValues[routeIndex];
            switch (midiSource.event()) {
            case Routing::MidiSource::Event::ControlAbsolute:
                if (message.controlNumber() == midiSource.controlNumber()) {
                    sourceValue = message.controlValue() * (1.f / 127.f);
                    consumed = true;
                }
                break;
            case Routing::MidiSource::Event::ControlRelative:
                if (message.controlNumber() == midiSource.controlNumber()) {
                    int value = message.controlValue();
                    value = value >= 64 ? 64 - value : value;
                    sourceValue = clamp(sourceValue + value * (1.f / 127.f), 0.f, 1.f);
                    consumed = true;
                }
                break;
            case Routing::MidiSource::Event::PitchBend:
                if (message.isPitchBend()) {
                    sourceValue = (message.pitchBend() + 0x2000) * (1.f / 16383.f);
                    consumed = true;
                }
                break;
            case Routing::MidiSource::Event::NoteMomentary:
                if (message.isNoteOn() && message.note() == midiSource.note()) {
                    sourceValue = 1.f;
                    consumed = true;
                } else if (message.isNoteOff() && message.note() == midiSource.note()) {
                    sourceValue = 0.f;
                    consumed = true;
                }
                break;
            case Routing::MidiSource::Event::NoteToggle:
                if (message.isNoteOn() && message.note() == midiSource.note()) {
                    sourceValue = sourceValue < 0.5f ? 1.f : 0.f;
                    consumed = true;
                }
                break;
            case Routing::MidiSource::Event::NoteVelocity:
                if (message.isNoteOn() && message.note() == midiSource.note()) {
                    sourceValue = message.velocity() * (1.f / 127.f);
                    consumed = true;
                }
                break;
            case Routing::MidiSource::Event::Last:
                break;
            }
        }
    }

    return consumed;
}

void RoutingEngine::updateSources() {
    for (int routeIndex = 0; routeIndex < CONFIG_ROUTE_COUNT; ++routeIndex) {
        const auto &route = _routing.route(routeIndex);
        if (route.active()) {
            auto &sourceValue = _sourceValues[routeIndex];
            switch (route.source()) {
            case Routing::Source::None:
                sourceValue = 0.f;
                break;
            case Routing::Source::CvIn1:
            case Routing::Source::CvIn2:
            case Routing::Source::CvIn3:
            case Routing::Source::CvIn4: {
                auto range = Types::voltageRangeInfo(route.cvSource().range());
                int index = int(route.source()) - int(Routing::Source::CvIn1);
                sourceValue = clamp((_engine.cvInput().channel(index) - range->lo) / (range->hi - range->lo), 0.f, 1.f);
                break;
            }
            case Routing::Source::CvOut1:
            case Routing::Source::CvOut2:
            case Routing::Source::CvOut3:
            case Routing::Source::CvOut4:
            case Routing::Source::CvOut5:
            case Routing::Source::CvOut6:
            case Routing::Source::CvOut7:
            case Routing::Source::CvOut8: {
                auto range = Types::voltageRangeInfo(route.cvSource().range());
                int index = int(route.source()) - int(Routing::Source::CvOut1);
                sourceValue = clamp((_engine.cvOutput().channel(index) - range->lo) / (range->hi - range->lo), 0.f, 1.f);
                break;
            }
            case Routing::Source::Midi:
                // handled in receiveMidi
                break;
            case Routing::Source::Last:
                break;
            }
        }
    }
}

void RoutingEngine::updateSinks() {
    for (int routeIndex = 0; routeIndex < CONFIG_ROUTE_COUNT; ++routeIndex) {
        const auto &route = _routing.route(routeIndex);
        if (route.active()) {
            float value = route.min() + _sourceValues[routeIndex] * (route.max() - route.min());
            // TODO handle pattern
            if (Routing::isTrackTarget(route.target()) || Routing::isSequenceTarget(route.target())) {
                uint8_t tracks = route.tracks();
                for (int trackIndex = 0; trackIndex < CONFIG_TRACK_COUNT; ++trackIndex) {
                    if (tracks & (1<<trackIndex)) {
                        _routing.writeTarget(route.target(), trackIndex, 0, value);
                    }
                }
            } else {
                _routing.writeTarget(route.target(), 0, 0, value);
            }
        }
    }
}
