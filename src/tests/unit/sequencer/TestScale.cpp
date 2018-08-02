#include "UnitTest.h"

#include "core/utils/StringBuilder.h"

#include "apps/sequencer/model/Scale.cpp"
#include "apps/sequencer/model/UserScale.cpp"

#include <array>
#include <cstdint>

UNIT_TEST("Scale") {

    CASE("noteName/noteVolts") {
        for (int i = 0; i < Scale::Count; ++i) {
            const auto &scale = Scale::get(i);
            int notesPerOctave = scale.notesPerOctave();

            DBG("----------------------------------------");
            DBG("%s", Scale::name(i));
            DBG("octave has %d notes", notesPerOctave);
            DBG("----------------------------------------");
            DBG("%-8s %-8s %-8s %-8s %-8s", "note", "volts", "short1", "short2", "long");

            for (int note = -notesPerOctave; note <= notesPerOctave; ++note) {
                FixedStringBuilder<8> short1Name;
                FixedStringBuilder<8> short2Name;
                FixedStringBuilder<16> longName;
                scale.noteName(short1Name, note, Scale::Short1);
                scale.noteName(short2Name, note, Scale::Short2);
                scale.noteName(longName, note, Scale::Long);
                float volts = scale.noteVolts(note);
                DBG("%-8d %-8.3f %-8s %-8s %-8s", note, volts, (const char *)(short1Name), (const char *)(short2Name), (const char *)(longName));
                // DBG("note = %d, volts = %f", note, volts);
            }
        }
    }

    CASE("noteFromVolts") {
        for (int i = 0; i < Scale::Count; ++i) {
            const auto &scale = Scale::get(i);
            int notesPerOctave = scale.notesPerOctave();

            DBG("----------------------------------------");
            DBG("%s", Scale::name(i));
            DBG("octave has %d notes", notesPerOctave);
            DBG("----------------------------------------");
            DBG("%-8s %-8s %-8s %-8s %-8s %-8s", "voltsin", "note", "volts", "short1", "short2", "long");

            for (int index = -12; index <= 12; ++index) {
                float voltsin = index * (1.f / 12.f);
                int note = scale.noteFromVolts(voltsin);
                FixedStringBuilder<8> short1Name;
                FixedStringBuilder<8> short2Name;
                FixedStringBuilder<16> longName;
                scale.noteName(short1Name, note, Scale::Short1);
                scale.noteName(short2Name, note, Scale::Short2);
                scale.noteName(longName, note, Scale::Long);
                float volts = scale.noteVolts(note);
                DBG("%-8.3f %-8d %-8.3f %-8s %-8s %-8s", voltsin, note, volts, (const char *)(short1Name), (const char *)(short2Name), (const char *)(longName));
            }

        }
    }

#ifdef PLATFORM_SIM

    CASE("markdown") {
        DBG("----------------------------------------");
        for (int i = 0; i < Scale::Count - 4; ++i) {
            const auto &scale = Scale::get(i);
            int notesPerOctave = scale.notesPerOctave();

            DBG("<h4>%s</h4>", Scale::name(i));
            FixedStringBuilder<4096> indices("| Index |");
            FixedStringBuilder<4096> separators("| :--- |");
            FixedStringBuilder<4096> names("| Name |");
            FixedStringBuilder<4096> volts("| Volts |");
            for (int note = 0; note < notesPerOctave; ++note) {
                FixedStringBuilder<8> name;
                if (scale.isChromatic()) {
                    scale.noteName(name, note, Scale::Short1);
                } else {
                    scale.noteName(name, note, Scale::Short2);
                }
                indices(" %d |", note + 1);
                separators(" --- |");
                names(" %s |", (const char *)(name));
                volts(" %.3f |", scale.noteVolts(note));
            }
            DBG("");
            DBG("%s", (const char *)(indices));
            DBG("%s", (const char *)(separators));
            DBG("%s", (const char *)(names));
            DBG("%s", (const char *)(volts));
            DBG("");
        }
        DBG("----------------------------------------");
    }

#endif // PLATFORM_SIM

}
