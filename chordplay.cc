#include <algorithm>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <popt.h>
#include <RtMidi.h>
#include "chordparser.h"

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


struct Voicing {
    MidiNote notes[5];
};


void print_voicing(const Voicing& voicing)
{
    const char* notenames[]={ "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "Bb", "B-" };

    printf("\e[31;1m%s%d    \e[32;1m%s%d  %s%d  %s%d   \e[36;1m%s%d\e[0m\n",
        notenames[voicing.notes[0]%12], voicing.notes[0]/12,
        notenames[voicing.notes[1]%12], voicing.notes[1]/12,
        notenames[voicing.notes[2]%12], voicing.notes[2]/12,
        notenames[voicing.notes[3]%12], voicing.notes[3]/12,
        notenames[voicing.notes[4]%12], voicing.notes[4]/12);
}


void play_voicing(MidiOut& midiout, const Voicing& voicing)
{
    print_voicing(voicing);

    midiout.note_on(0, voicing.notes[0], 64);
    midiout.note_on(1, voicing.notes[1], 64);
    midiout.note_on(1, voicing.notes[2], 64);
    midiout.note_on(1, voicing.notes[3], 64);
    midiout.note_on(2, voicing.notes[4], 64);
}


void stop_voicing(MidiOut& midiout, const Voicing& voicing)
{
    midiout.note_off(0, voicing.notes[0], 64);
    midiout.note_off(1, voicing.notes[1], 64);
    midiout.note_off(1, voicing.notes[2], 64);
    midiout.note_off(1, voicing.notes[3], 64);
    midiout.note_off(2, voicing.notes[4], 64);
}


std::vector<Voicing> enumerate_voicings(const Chord& chord)
{
    std::vector<Voicing> result;

    std::vector<MidiNote> noteset[5];

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
        int v=v1.notes[i] - v2.notes[i];
        cost+=v*v;
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

    printf("voice leading cost=%d\n", bestcost);

    int j=best;

    std::vector<Voicing> result;

    while (i>=0) {
        result.push_back(pathnodes[i][j].voicing);
        j=pathnodes[i--][j].back;
    }

    std::reverse(result.begin(), result.end());
    return result;
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

    for (const Chord& ch: chords) {
        std::cout << ch.get_name() << ':' << std::endl;

        for (const Note& n: ch.notes)
            if (n)
                std::cout << '\t' << n.get_name() << std::endl;
    }

    std::vector<Voicing> voiceleading=compute_voice_leading(chords);

    for (auto& v: voiceleading)
        print_voicing(v);

    //return 0;

    try {
        RtMidiOut rtmidiout;

        const int numports=rtmidiout.getPortCount();
        printf("%d midi ports:\n", numports);
        for (int i=0;i<numports;i++)
            printf("%d: %s\n", i, rtmidiout.getPortName(i).c_str());

        rtmidiout.openPort(4);

        MidiOut midiout(rtmidiout);
        midiout.program_change(0, 43);
        midiout.program_change(1, 42);
        midiout.program_change(2, 41);

        for (auto& v: voiceleading) {
            play_voicing(midiout, v);
            sleep(2);
            stop_voicing(midiout, v);
        }
    }
    catch (const RtMidiError& err) {
        err.printMessage();
    }

    poptFreeContext(pctx);

    return 0;
}
