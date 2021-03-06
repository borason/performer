#include "Config.h"

#include "Ui.h"
#include "Key.h"

#include "core/Debug.h"
#include "core/profiler/Profiler.h"
#include "core/utils/StringBuilder.h"

#include "model/Model.h"

Ui::Ui(Model &model, Engine &engine, Lcd &lcd, ButtonLedMatrix &blm, Encoder &encoder) :
    _model(model),
    _engine(engine),
    _lcd(lcd),
    _blm(blm),
    _encoder(encoder),
    _frameBuffer(CONFIG_LCD_WIDTH, CONFIG_LCD_HEIGHT, _frameBufferData),
    _canvas(_frameBuffer),
    _pageManager(_pages),
    _pageContext({ _messageManager, _keyState, _globalKeyState, _model, _engine }),
    _pages(_pageManager, _pageContext),
    _controllerManager(model, engine)
{
}

void Ui::init() {
    _keyState.reset();
    _globalKeyState.reset();

    _pageManager.push(&_pages.top);
    _pages.top.init();
#ifdef CONFIG_ENABLE_INTRO
    _pageManager.push(&_pages.intro);
#endif

    _engine.setMidiReceiveHandler([this] (MidiPort port, const MidiMessage &message) {
        if (!_midiMessages.writable()) {
            DBG("ui midi buffer overflow");
            _midiMessages.read();
        }
        _midiMessages.write({ port, message });
        return port == MidiPort::UsbMidi && _controllerManager.isConnected();
    });

    _engine.setUsbMidiConnectHandler([this] (uint16_t vendorId, uint16_t productId) {
        _messageManager.showMessage("USB MIDI DEVICE CONNECTED");
        _controllerManager.connect(vendorId, productId);
    });

    _engine.setUsbMidiDisconnectHandler([this] () {
        _messageManager.showMessage("USB MIDI DEVICE DISCONNECTED");
        _controllerManager.disconnect();
    });

    _engine.setMessageHandler([this] (const char *text, uint32_t duration) {
        _messageManager.showMessage(text, duration);
    });

    _lastUpdateTicks = os::ticks();
}

void Ui::update() {
    handleKeys();
    handleEncoder();
    handleMidi();

    _leds.clear();
    _pageManager.updateLeds(_leds);
    _blm.setLeds(_leds.array());

    // update display at target fps
    uint32_t currentTicks = os::ticks();
    uint32_t intervalTicks = os::time::ms(1000 / _pageManager.fps());
    if (currentTicks - _lastUpdateTicks >= intervalTicks) {
        _pageManager.draw(_canvas);
        _messageManager.update();
        _messageManager.draw(_canvas);
        _lcd.draw(_frameBuffer.data());
        _lastUpdateTicks += intervalTicks;
    }

    if (!_engine.isLocked()) {
        _controllerManager.update();
    }
}

void Ui::showAssert(const char *filename, int line, const char *msg) {
    _canvas.setColor(0);
    _canvas.fill();

    _canvas.setColor(0xf);
    _canvas.setFont(Font::Small);
    _canvas.drawText(4, 10, "FATAL ERROR");

    _canvas.setFont(Font::Tiny);
    _canvas.drawTextMultiline(4, 20, CONFIG_LCD_WIDTH - 8, msg);

    if (filename) {
        FixedStringBuilder<128> str("%s:%d", filename, line);
        _canvas.drawTextMultiline(4, 40, CONFIG_LCD_WIDTH - 8, str);
    }

    _lcd.draw(_frameBuffer.data());
}

void Ui::handleKeys() {
    ButtonLedMatrix::Event event;
    while (_blm.nextEvent(event)) {
        bool isDown = event.action() == ButtonLedMatrix::Event::KeyDown;
        _keyState[event.value()] = isDown;
        _globalKeyState[event.value()] = isDown;
        Key key(event.value(), _globalKeyState);
        KeyEvent keyEvent(isDown ? Event::KeyDown : Event::KeyUp, key);
        _pageManager.dispatchEvent(keyEvent);
        if (isDown) {
            KeyPressEvent keyPressEvent = _keyPressEventTracker.process(key);
            _pageManager.dispatchEvent(keyPressEvent);
        }
    }
}

void Ui::handleEncoder() {
    Encoder::Event event;
    while (_encoder.nextEvent(event)) {
        switch (event) {
        case Encoder::Left:
        case Encoder::Right: {
            EncoderEvent encoderEvent(event == Encoder::Left ? -1 : 1, _keyState[Key::Encoder]);
            _pageManager.dispatchEvent(encoderEvent);
            break;
        }
        case Encoder::Down:
        case Encoder::Up: {
            bool isDown = event == Encoder::Down;
            _keyState[Key::Encoder] = isDown ? 1 : 0;
            _globalKeyState[Key::Encoder] = isDown ? 1 : 0;
            Key key(Key::Encoder, _globalKeyState);
            KeyEvent keyEvent(isDown ? Event::KeyDown : Event::KeyUp, key);
            _pageManager.dispatchEvent(keyEvent);
            if (isDown) {
                KeyPressEvent keyPressEvent = _keyPressEventTracker.process(key);
                _pageManager.dispatchEvent(keyPressEvent);
            }
            break;
        }
        }
    }
}

void Ui::handleMidi() {
    while (_midiMessages.readable()) {
        auto item = _midiMessages.read();
        if (!_controllerManager.recvMidi(item.first, item.second)) {
            MidiEvent midiEvent(item.first, item.second);
            _pageManager.dispatchEvent(midiEvent);
        }
    }
}
