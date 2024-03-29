#include <queue>
#include <unistd.h>
#include <math.h>
#include "midi.h"
#include "note.h"


Sequencer::Track::Track(int8_t channel, int8_t transposition):channel(channel), transposition(transposition)
{
}


void Sequencer::Track::append_note(float timestamp, const Note& note, uint8_t vel)
{
    events.push_back(Event { timestamp, uint8_t(note.get_midi_note()+transposition), vel });
}


void Sequencer::Track::append_note(float timestamp, uint8_t note, uint8_t vel)
{
    events.push_back(Event { timestamp, uint8_t(note+transposition), vel });
}


void Sequencer::Track::append_pause(float timestamp)
{
    events.push_back(Event { timestamp, 0xff, 0 });
}


Sequencer::Sequencer(MidiOut& midiout, int bpm, int transposition):midiout(midiout), bpm(bpm), transposition(transposition)
{
}


Sequencer::Track* Sequencer::add_track(int8_t channel, int8_t program)
{
    midiout.program_change(channel, program);

    Track* track=new Track(channel, channel==9 ? 0 : transposition);
    tracks.push_back(track);

    return track;
}


void Sequencer::play(bool loop)
{
    struct Event {
        Track*  track;
        int     index;
    };

    struct CompareEvent {
        bool operator()(const Event& lhs, const Event& rhs) const
        {
            return lhs.track->events[lhs.index].timestamp > rhs.track->events[rhs.index].timestamp;
        }
    };

    std::priority_queue<Event, std::vector<Event>, CompareEvent> queue;

    do {
        for (Track* track: tracks)
            if (!track->events.empty())
                queue.push(Event { track, 0 });
        
        float curtime=0.0f;

        while (!queue.empty()) {
            Event ev=queue.top();
            queue.pop();

            const auto& trev=ev.track->events[ev.index];

            if (trev.timestamp>curtime && usleep(lrintf((trev.timestamp-curtime)*60000000.0f/bpm))<0) {
                for (Track* track: tracks)
                    if (track->curnote>=0)
                        midiout.note_off(track->channel, track->curnote, 0);

                return;
            }

            curtime=trev.timestamp;

            if (ev.track->curnote>=0)
                midiout.note_off(ev.track->channel, ev.track->curnote, 0);

            if (trev.velocity>0) {
                midiout.note_on(ev.track->channel, trev.note, trev.velocity);
                ev.track->curnote=trev.note;
            }
            else
                ev.track->curnote=-1;

            if (++ev.index<ev.track->events.size())
                queue.push(ev);
        }
    } while (loop);
}


void Sequencer::stop()
{
    // noop - currently we rely on usleep returning an error upon a signal
}
