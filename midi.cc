#include <algorithm>
#include <unistd.h>
#include <math.h>
#include "midi.h"
#include "note.h"


void Sequencer::sequence_note(const Ensemble::Voice& voice, float timestamp, const Note& note)
{
    Event event;

    event.timestamp=timestamp;
    event.id=voice.id;
    event.channel=voice.midi_channel;
    event.note=note.get_midi_note();
    event.vel=voice.midi_velocity;

    events.push_back(event);
}


void Sequencer::sequence_pause(const Ensemble::Voice& voice, float timestamp)
{
    Event event;

    event.timestamp=timestamp;
    event.id=voice.id;
    event.channel=voice.midi_channel;
    event.note=-1;
    event.vel=0;

    events.push_back(event);
}


void Sequencer::play(MidiOut& midiout)
{
    std::sort(events.begin(), events.end(), [](const Event& lhs, const Event& rhs) { return lhs.timestamp<rhs.timestamp; });

    int8_t playing[8] { -1, -1, -1, -1, -1, -1, -1, -1};
    int8_t channel[8];  // FIXME: we shouldn't have to remember channels
    float curtime=0.0f;

    for (auto& ev: events) {
        float delay=ev.timestamp - curtime;
        curtime=ev.timestamp;

        if (delay>0)
            usleep(lrintf(delay*800000));
        
        if (playing[ev.id]>=0)
            midiout.note_off(ev.channel, playing[ev.id], 64);

        if (ev.note>=0)
            midiout.note_on(ev.channel, ev.note, ev.vel);

        playing[ev.id]=ev.note;
        channel[ev.id]=ev.channel;
    }

    for (int i=0;i<8;i++)
        if (playing[i]>=0)
            midiout.note_off(channel[i], playing[i], 64);
}

