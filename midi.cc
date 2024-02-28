#include <algorithm>
#include <unistd.h>
#include <math.h>
#include "midi.h"
#include "note.h"


void Sequencer::sequence_note(int voice, float timestamp, const Note& note, int vel)
{
    Event event;

    event.timestamp=timestamp;
    event.voice=voice;
    event.note=note.get_midi_note();
    event.vel=vel;

    events.push_back(event);
}


void Sequencer::sequence_pause(int voice, float timestamp)
{
    Event event;

    event.timestamp=timestamp;
    event.voice=voice;
    event.note=-1;
    event.vel=0;

    events.push_back(event);
}


void Sequencer::play(MidiOut& midiout)
{
    std::sort(events.begin(), events.end(), [](const Event& lhs, const Event& rhs) { return lhs.timestamp<rhs.timestamp; });

    int8_t playing[8] { -1, -1, -1, -1, -1, -1, -1, -1};
    float curtime=0.0f;

    for (auto& ev: events) {
        float delay=ev.timestamp - curtime;
        curtime=ev.timestamp;

        if (delay>0)
            usleep(lrintf(delay*800000));
        
        if (playing[ev.voice]>=0)
            midiout.note_off(voicechannel[ev.voice], playing[ev.voice], 64);

        if (ev.note>=0)
            midiout.note_on(voicechannel[ev.voice], ev.note, ev.vel);

        playing[ev.voice]=ev.note;
    }

    for (int i=0;i<8;i++)
        if (playing[i]>=0)
            midiout.note_off(voicechannel[i], playing[i], 64);
}

