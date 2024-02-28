#ifndef INCLUDE_MIDI_H
#define INCLUDE_MIDI_H

#include <RtMidi.h>

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
        int8_t  voice;
        int8_t  note;
        int8_t  vel;
    };

    std::vector<Event>  events;

    const int8_t voicechannel[8] { 0, 1, 1, 1, 2, 3, 4, 5 };

public:
    void sequence_note(int voice, float timestamp, const Note& note, int vel);
    void sequence_pause(int voice, float timestamp);

    void play(MidiOut&);
};



#endif
