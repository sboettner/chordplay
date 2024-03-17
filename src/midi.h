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
public:
    class Track {
        friend class Sequencer;

        struct Event {
            float   timestamp;
            uint8_t note;
            uint8_t velocity;
        };

        std::vector<Event> events;

        int8_t  channel;
        int8_t  curnote=-1;

        Track(int8_t channel);

    public:
        void append_note(float timestamp, const Note& note, uint8_t vel);
        void append_note(float timestamp, uint8_t note, uint8_t vel);
        void append_pause(float timestamp);
    };

    Sequencer(MidiOut&, int bpm, int transposition);

    Track* add_track(int8_t channel, int8_t program);

    void play(bool loop);
    void stop();

private:
    MidiOut&    midiout;

    std::vector<Track*> tracks;

    int transposition=0;
    int bpm=0;

};



#endif
