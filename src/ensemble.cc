#include <iostream>
#include "ensemble.h"
#include "chord.h"
#include "scale.h"
#include "midi.h"


Ensemble::Voicing::Voicing(int numvoices):numvoices(numvoices)
{
    notes=new Note[numvoices];
}


Ensemble::Voicing::Voicing(Voicing&& other)
{
    numvoices=other.numvoices;
    notes=other.notes;

    other.numvoices=0;
    other.notes=nullptr;
}


Ensemble::Voicing::~Voicing()
{
    delete[] notes;
}


Ensemble::Voicing& Ensemble::Voicing::operator=(Voicing&& other)
{
    delete[] notes;

    numvoices=other.numvoices;
    notes=other.notes;

    other.numvoices=0;
    other.notes=nullptr;

    return *this;
}


void Ensemble::add_voice(const Voice& v)
{
    if (v.role==Voice::Role::Melody) {
        melody_voices.push_back(v);
        melody_voices.back().id=harmony_voices.size() + melody_voices.size() - 1;
    }
    else {
        harmony_voices.push_back(v);
        harmony_voices.back().id=harmony_voices.size() + melody_voices.size() - 1;
    }
}


void Ensemble::init_midi_programs(MidiOut& midi) const
{
    for (const Voice& v: harmony_voices)
        midi.program_change(v.midi_channel, v.midi_program);

    for (const Voice& v: melody_voices)
        midi.program_change(v.midi_channel, v.midi_program);
}


std::vector<Ensemble::Voicing> Ensemble::enumerate_harmony_voicings(const Chord& chord) const
{
    std::vector<Voicing> result;

    const int n=harmony_voices.size();

    std::vector<Note>* noteset=new std::vector<Note>[n];
    
    int* seq=new int[n];
    for (int i=0;i<n;i++)
        seq[i]=0;

    for (int i=0;i<n;i++) {
        if (harmony_voices[i].role==Voice::Role::Bass) {
            NoteClass bassnote=chord.bass ? chord.bass : chord.notes[0];

            for (int k=0;k<10;k++) {
                Note note(bassnote, k);
                
                if (note<harmony_voices[i].range_low) continue;
                if (note>harmony_voices[i].range_high) break;

                noteset[i].push_back(note);
            }
        }
        else {
            for (int j=0;j<6 && chord.notes[j];j++) {
                for (int k=0;k<10;k++) {
                    Note note(chord.notes[j], k);
                    
                    if (note<harmony_voices[i].range_low) continue;
                    if (note>harmony_voices[i].range_high) break;

                    noteset[i].push_back(note);
                }
            }
        }
    }

    for (;;) {
        Voicing voicing(n);
        uint8_t havenotes=0;

        for (int i=0;i<n;i++) {
            voicing[i]=noteset[i][seq[i]];

            for (int j=0;j<6;j++)
                if (chord.notes[j]==voicing[i])
                    havenotes|=1<<j;
        }

        bool monotonic=true;
        for (int i=1;i<n;i++)
            if (voicing[i-1]>=voicing[i])
                monotonic=false;

        if (monotonic && !(chord.required&~havenotes))
            result.push_back(std::move(voicing));

        int i=0;
        while (i<n) {
            if (++seq[i]<noteset[i].size())
                break;

            seq[i++]=0;
        }

        if (i==n) break;
    }

    delete[] noteset;
    delete[] seq;

    return result;
}


void Ensemble::print_harmony_voicing(const Chord& chord, const Scale& scale, const Voicing& voicing) const
{
    const char* colorcodes[8]={
        "\e[90m",
        "\e[94m",
        "\e[92m",
        "\e[96m",
        "\e[91m",
        "\e[95m",
        "\e[93m",
        "\e[97m"
    };

    std::cout << "\e[95;1m" << chord.get_name() << '\t' << scale.get_name() << "\t\e[0m";

    for (int i=0;i<harmony_voices.size();i++)
        std::cout << colorcodes[harmony_voices[i].color] << voicing[i].get_name() << '\t';

    std::cout << "\e[0m" << std::endl;
}
