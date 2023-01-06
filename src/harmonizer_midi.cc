#include "harmonizer_midi.h"

#include <iomanip>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>

#include "rtmidi/RtMidi.h"

class HarmonizerMidi {
  public:
    HarmonizerMidi(harmonizer_midi_params_t *params) {
        _midi_in = std::make_unique<RtMidiIn>();

        unsigned int nPorts = _midi_in->getPortCount();
        int selectedPort = 0;
        if (nPorts == 0) {
            std::cout << "No ports available!\n";
            // TODO errors
        } else {
            for (int i = 0; i < nPorts; ++i) {
                std::string port_name = _midi_in->getPortName(i);
                std::cout << "Midi port [" << i << "]: " << port_name
                          << std::endl;
                if (port_name.find("Piano") != std::string::npos) {
                    selectedPort = i;
                }
            }
        }
        _midi_in->openPort(selectedPort);
        // Don't ignore sysex, timing, or active sensing messages.
        _midi_in->ignoreTypes(false, false, false);
    }
    ~HarmonizerMidi() {}

    int poll_events() {
        double stamp;
        std::vector<unsigned char> message;
        while (true) {
            stamp = _midi_in->getMessage(&message);
            if (message.size() == 0) {
                break;
            } else { // note
                /*for (int i = 0; i < message.size(); i++)
                    std::cout << "Byte " << i << " = " << (int)message[i]
                              << ", ";
                std::cout << std::endl;*/

                // parse midi message and push to events
                const int NOTE_ON = 144;
                const int NOTE_OFF = 128;

                if (message.size() == 3 &&
                    (message[0] == NOTE_ON || message[0] == NOTE_OFF)) {

                    std::lock_guard<std::mutex> guard(_evt_mutex);
                    _events.emplace_back();
                    auto &evt = _events.back();

                    evt.note_on = message[0] == NOTE_ON;
                    evt.pitch = (int)message[1];
                    evt.velocity = (int)message[2];
                }
            }
        }
        return 0;
    }

    int pop_event(harmonizer_midi_event_t *evt) {
        std::lock_guard<std::mutex> guard(_evt_mutex);
        if (_events.empty()) {
            return false;
        }
        *evt = _events.front();
        _events.pop_front();
        return true;
    }

  public:
    std::unique_ptr<RtMidiIn> _midi_in;
    std::list<harmonizer_midi_event_t> _events;
    std::mutex _evt_mutex;
};

std::unique_ptr<HarmonizerMidi> _midi;

int init_midi(harmonizer_midi_params_t *params) {
    // TODO handle errors
    _midi = std::make_unique<HarmonizerMidi>(params);
    return 0;
}

int poll_midi_events() { return _midi->poll_events(); }

int pop_midi_event(harmonizer_midi_event_t *evt) {
    return _midi->pop_event(evt);
}

int destroy_midi() {
    _midi = nullptr;
    return 0;
}
