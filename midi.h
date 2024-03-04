#ifndef INCLUDE_MIDI_H
#define INCLUDE_MIDI_H

#include <RtMidi.h>
#include "ensemble.h"

class MidiOut {
    RtMidiOut&  rtmidiout;

public:
    MidiOut(RtMidiOut& rtmidiout):rtmidiout(rtmidiout) {}

    void note_off(int ch, int note, int vel)
    {
        const uint8_t msg[3]={ uint8_t(0x80|ch), uint8_t(note), uint8_t(vel) };
        rtmidiout.sendMessage(msg, sizeof(msg));
    }

    void note_on(int ch, int note, int vel)
    {
        const uint8_t msg[3]={ uint8_t(0x90|ch), uint8_t(note), uint8_t(vel) };
        rtmidiout.sendMessage(msg, sizeof(msg));
    }

    void program_change(int ch, int prog)
    {
        const uint8_t msg[2]={ uint8_t(0xC0|ch), uint8_t(prog) };
        rtmidiout.sendMessage(msg, sizeof(msg));
    }
};


class Note;

class Sequencer {
    struct Event {
        float   timestamp;
        int8_t  id;
        int8_t  channel;
        int8_t  note;
        int8_t  vel;
    };

    std::vector<Event>  events;
    int transposition=0;
    int bpm=0;

public:
    void set_transposition(int);
    void set_bpm(int);

    void sequence_note(const Ensemble::Voice&, float timestamp, const Note& note);
    void sequence_pause(const Ensemble::Voice&, float timestamp);

    void play(MidiOut&);
    void stop();
};



#endif
