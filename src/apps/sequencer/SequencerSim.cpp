#include "Config.h"

#include "sim/Simulator.h"

#include "drivers/Adc.h"
#include "drivers/ButtonLedMatrix.h"
#include "drivers/ClockTimer.h"
#include "drivers/Dac.h"
#include "drivers/Dio.h"
#include "drivers/Encoder.h"
#include "drivers/GateOutput.h"
#include "drivers/Lcd.h"
#include "drivers/Midi.h"
#include "drivers/SdCard.h"
#include "drivers/UsbMidi.h"

#include "core/fs/Volume.h"

#include "model/Model.h"
#include "model/FileManager.h"
#include "engine/Engine.h"
#include "ui/Ui.h"

#include "os/os.h"

#ifdef __EMSCRIPTEN__
# include <emscripten.h>
#endif

static os::PeriodicTask<1024> fsTask("file", CONFIG_FILE_TASK_PRIORITY, os::time::ms(10), [] () {
    FileManager::processTask();
});

struct Environment {
    sim::Simulator &simulator;

    // drivers
    ClockTimer clockTimer;
    ButtonLedMatrix blm;
    Lcd lcd;
    Adc adc;
    Dac dac;
    Dio dio;
    Encoder encoder;
    GateOutput gateOutput;
    Midi midi;
    UsbMidi usbMidi;
    SdCard sdCard;

    // filesystem
    fs::Volume volume;

    // application
    Model model;
    Engine engine;
    Ui ui;

    Environment() :
        simulator(sim::Simulator::instance()),

        volume(sdCard),

        engine(model, clockTimer, adc, dac, dio, gateOutput, midi, usbMidi),
        ui(model, engine, lcd, blm, encoder)
    {
        model.init();
        engine.init();
        ui.init();
    }

    bool terminate() {
        return simulator.terminate();
    }

    void update() {
        simulator.update();
        engine.update();
        ui.update();
        simulator.render();
#ifndef __EMSCRIPTEN__
        simulator.delay(1);
#endif // __EMSCRIPTEN__
    }

};

#ifdef __EMSCRIPTEN__
static Environment *g_env;
static void mainLoop() {
    g_env->update();
}
#endif // __EMSCRIPTEN__

int main(int argc, char *argv[]) {
    sdl::Init init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);

    Environment env;

#ifdef __EMSCRIPTEN__
    // 0 fps means to use requestAnimationFrame; non-0 means to use setTimeout.
    g_env = &env;
    emscripten_set_main_loop(mainLoop, 0, 1);
#else // __EMSCRIPTEN__
    while (!env.terminate()) {
        env.update();
    }
#endif // __EMSCRIPTEN__

    return 0;
}
