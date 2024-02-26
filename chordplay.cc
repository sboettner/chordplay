#include <algorithm>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>
#include <popt.h>
#include <RtMidi.h>
#include "chordparser.h"
#include "scale.h"

poptOption option_table[]={
    POPT_AUTOHELP
    POPT_TABLEEND
};


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

struct Voicing {
    Note notes[5];
};


void print_voicing(const Voicing& voicing)
{
    const char* colorcodes[5]={ "\e[31;1m", "\e[32;1m", "\e[32;1m", "\e[32;1m", "\e[36;1m" };

    for (int i=0;i<5;i++)
        std::cout << colorcodes[i] << voicing.notes[i].get_name() << '\t';

    std::cout << "\e[0m" << std::endl;
}

std::vector<Voicing> enumerate_voicings(const Chord& chord)
{
    std::vector<Voicing> result;

    std::vector<Note> noteset[5];

    if (chord.bass)
        noteset[0].emplace_back(chord.bass, 3);
    else
        noteset[0].emplace_back(chord.notes[0], 3);

    for (int i=1;i<5;i++) {
        for (int j=0;j<6;j++)
            if (chord.notes[j])
                for (int k=4;k<=7;k++)
                    noteset[i].emplace_back(chord.notes[j], k);
    }

    int seq[5]={};

    do {
        Voicing voicing;
        uint8_t havenotes=0;

        for (int i=0;i<5;i++) {
            voicing.notes[i]=noteset[i][seq[i]];

            for (int j=0;j<6;j++)
                if (chord.notes[j]==voicing.notes[i])
                    havenotes|=1<<j;
        }

        bool monotonic=true;
        for (int i=1;i<5;i++)
            if (voicing.notes[i-1]>=voicing.notes[i])
                monotonic=false;

        if (monotonic && !(chord.required&~havenotes))
            result.push_back(voicing);

        for (int i=0;i<5;i++)       
            if (++seq[i]<noteset[i].size()) break; else seq[i]=0;
    } while (seq[0] | seq[1] | seq[2] | seq[3] | seq[4]);

    return result;
}


int compute_voice_leading_cost(const Voicing& v1, const Voicing& v2)
{
    int cost=0;

    for (int i=0;i<5;i++) {
        int v=v1.notes[i].get_midi_note() - v2.notes[i].get_midi_note();
        cost+=v*v;
    }

    for (int i=1;i<5;i++) {
        for (int j=0;j<i;j++) {
            int v=v1.notes[i].get_midi_note() - v1.notes[j].get_midi_note();
            if (v%12!=0 && v%12!=7) continue;

            if (v2.notes[i].get_midi_note()==v2.notes[j].get_midi_note()+v)
                cost+=1000; // forbidden parallel
        }
    }

    return cost;
}


std::vector<Voicing> compute_voice_leading(const std::vector<Chord>& chords)
{
    struct pathnode_t {
        Voicing voicing;
        int     back;
        int     cost;
    };

    std::vector<std::vector<pathnode_t>> pathnodes;

    for (const Chord& ch: chords) {
        std::vector<pathnode_t> voicings;

        for (const Voicing& v: enumerate_voicings(ch))
            voicings.push_back(pathnode_t { v, -1, 0 });

        pathnodes.push_back(std::move(voicings));
    }

    for (int i=1;i<pathnodes.size();i++) {
        for (int j=0;j<pathnodes[i].size();j++) {
            pathnodes[i][j].cost=INT_MAX;

            for (int k=0;k<pathnodes[i-1].size();k++) {
                int cost=pathnodes[i-1][k].cost + compute_voice_leading_cost(pathnodes[i-1][k].voicing, pathnodes[i][j].voicing);

                if (cost<pathnodes[i][j].cost) {
                    pathnodes[i][j].cost=cost;
                    pathnodes[i][j].back=k;
                }
            }
        }
    }

    int bestcost=INT_MAX;
    int best=0;

    int i=pathnodes.size()-1;
    for (int j=0;j<pathnodes[i].size();j++) {
        if (pathnodes[i][j].cost<bestcost) {
            bestcost=pathnodes[i][j].cost;
            best=j;
        }
    }

    int j=best;

    std::vector<Voicing> result;

    while (i>=0) {
        result.push_back(pathnodes[i][j].voicing);
        j=pathnodes[i--][j].back;
    }

    std::reverse(result.begin(), result.end());
    return result;
}


std::vector<Note> improvise_melody(const std::vector<Chord>& chords)
{
    std::vector<Note> melody;
    std::vector<std::vector<Note>> noteset;

    const int n=chords.size()*2 - 1;

    for (int i=0;i<n;i++) {
        melody.emplace_back(chords[i/2].notes[0], 5);

        std::vector<Note> notes;
        for (int j=0;j<6 && chords[i/2].notes[j];j++) {
            notes.emplace_back(chords[i/2].notes[j], 4);
            notes.emplace_back(chords[i/2].notes[j], 5);
            notes.emplace_back(chords[i/2].notes[j], 6);
        }

        noteset.push_back(std::move(notes));
    }

    struct NoteCandidate {
        Note    note;
        float   cumul;
    };

    srand(time(nullptr));

    for (int pass=0;pass<10;pass++) {
        for (int i=1;i+1<n;i++) {
            std::vector<NoteCandidate> candidates;
            float cumul=0.0f;

            for (const Note& note: noteset[i]) {
                auto Pr=[](int d) { return expf(-0.75f*abs(d))*abs(d); };

                float prob=Pr(note.get_midi_note() - melody[i-1].get_midi_note()) * Pr(note.get_midi_note() - melody[i+1].get_midi_note());
                printf("%s %s %s: %f (%f)\n", melody[i-1].get_name().c_str(), note.get_name().c_str(), melody[i+1].get_name().c_str(), prob, cumul);

                NoteCandidate nc;
                nc.note=note;
                nc.cumul=cumul;
                cumul+=prob;

                candidates.push_back(nc);
            }

            float v=cumul * ldexpf(rand()&0xfffff, -20);
            int c=0;
            while (c+1<candidates.size() && candidates[c+1].cumul<=v) c++;

            printf("total=%f  v=%f  c=%d\n", cumul, v, c);

            melody[i]=candidates[c].note;
        }
    }

    return melody;
}


std::vector<Note> improvise_passing_notes(const std::vector<Note>& in_melody, const std::vector<Chord>& chords)
{
    std::vector<Note> melody;

    for (int i=0;i+1<in_melody.size();i++) {
        melody.push_back(in_melody[i]);

        Scale chordscale(chords[i/2]);

        int8_t prev=chordscale.to_scale(in_melody[i]);
        int8_t next=chordscale.to_scale(in_melody[i+1]);

        switch (next-prev) {
        case -3:
        case -2:
            melody.push_back(chordscale(prev-1));
            break;
        case -4:
        case -1:
            melody.push_back(chordscale(prev-2));
            break;
        case 1:
        case 4:
            melody.push_back(chordscale(prev+2));
            break;
        case 0:
        case 2:
        case 3:
            melody.push_back(chordscale(prev+1));
            break;
        default:
            melody.push_back(in_melody[i]);
        }
    }

    melody.push_back(in_melody.back());

    return melody;
}


int main(int argc, const char* argv[])
{
    poptContext pctx=poptGetContext(NULL, argc, argv, option_table, 0);

    while (poptGetNextOpt(pctx)>=0);

    ChordParser parsechord;
    std::vector<Chord> chords;

    while (const char* arg=poptGetArg(pctx)) {
        auto chord=parsechord(arg);
        if (chord.has_value())
            chords.push_back(*chord);
        else {
            printf("Error: invalid chord '%s'\n", arg);
            return 1;
        }
    }

    if (chords.empty()) {
        printf("Error: no chords given\n");
        return 1;
    }

    const Chord& ch=chords[0];
    Scale scale(ch);

    std::vector<Voicing> voiceleading=compute_voice_leading(chords);

    for (auto& v: voiceleading)
        print_voicing(v);

    std::vector<Note> melody=improvise_melody(chords);
    melody=improvise_passing_notes(melody, chords);

    for (int i=0;i<melody.size();i++)
        std::cout << (i&1 ? "\e[0m" : "\e[1m") << melody[i].get_name() << "\e[0m" << std::endl;


    Sequencer seq;

    for (int i=0;i<voiceleading.size();i++) {
        print_voicing(voiceleading[i]);
        
        for (int j=0;j<5;j++)
            seq.sequence_note(j, 2.0f*i, voiceleading[i].notes[j], 64);
    }

    for (int j=0;j<5;j++)
        seq.sequence_pause(j, voiceleading.size()*2.0f);

    const float melody_timing[4]={ 0.0f, 0.75f, 1.0f, 1.75f };
    for (int i=0;i<melody.size();i++)
        seq.sequence_note(5, (i&~3)*0.5f + melody_timing[i&3], melody[i], 96);
    
    seq.sequence_pause(5, melody.size()*0.5f);


    try {
        RtMidiOut rtmidiout;

        const int numports=rtmidiout.getPortCount();
        int midiport=-1;

        for (int i=0;i<numports;i++) {
            std::string portname=rtmidiout.getPortName(i).c_str();
            std::transform(portname.begin(), portname.end(), portname.begin(), tolower);
            if (portname.find("synth")!=std::string::npos) {
                midiport=i;
                break;
            }
        }

        if (midiport<0) {
            std::cerr << "Error: No MIDI synth found" << std::endl;
            return 1;
        }

        std::cout << "Using MIDI port: " << rtmidiout.getPortName(midiport).c_str() << std::endl;

        rtmidiout.openPort(midiport);

        MidiOut midiout(rtmidiout);
        midiout.program_change(0, 43);
        midiout.program_change(1, 42);
        midiout.program_change(2, 41);
        midiout.program_change(3, 72);

        seq.play(midiout);
    }
    catch (const RtMidiError& err) {
        err.printMessage();
    }

    poptFreeContext(pctx);

    return 0;
}
