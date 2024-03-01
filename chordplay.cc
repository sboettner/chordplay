#include <algorithm>
#include <fstream>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <popt.h>
#include "chordparser.h"
#include "ensembleparser.h"
#include "scale.h"
#include "midi.h"

int opt_play=0;

poptOption option_table[]={
    { NULL, 'p', POPT_ARG_NONE, &opt_play, 0, "Play using MIDI output", NULL },
    POPT_AUTOHELP
    POPT_TABLEEND
};


int compute_voice_leading_cost(const Ensemble::Voicing& v1, const Ensemble::Voicing& v2)
{
    const int n=v1.get_voice_count();

    int cost=0;

    for (int i=0;i<n;i++) {
        int v=v1[i].get_midi_note() - v2[i].get_midi_note();
        cost+=v*v;
    }

    for (int i=1;i<n;i++) {
        for (int j=0;j<i;j++) {
            int v=v1[i].get_midi_note() - v1[j].get_midi_note();
            if (v%12!=0 && v%12!=7) continue;

            if (v2[i].get_midi_note()==v2[j].get_midi_note()+v)
                cost+=1000; // forbidden parallel
        }
    }

    return cost;
}


std::vector<Ensemble::Voicing> compute_voice_leading(const Ensemble& ensemble, const std::vector<Chord>& chords)
{
    struct pathnode_t {
        Ensemble::Voicing   voicing;
        int                 back;
        int                 cost;
    };

    std::vector<std::vector<pathnode_t>> pathnodes;

    for (const Chord& ch: chords) {
        std::vector<pathnode_t> voicings;

        for (Ensemble::Voicing& v: ensemble.enumerate_harmony_voicings(ch))
            voicings.push_back(pathnode_t { std::move(v), -1, 0 });

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

    std::vector<Ensemble::Voicing> result;

    while (i>=0) {
        result.push_back(std::move(pathnodes[i][j].voicing));
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
                //printf("%s %s %s: %f (%f)\n", melody[i-1].get_name().c_str(), note.get_name().c_str(), melody[i+1].get_name().c_str(), prob, cumul);

                NoteCandidate nc;
                nc.note=note;
                nc.cumul=cumul;
                cumul+=prob;

                candidates.push_back(nc);
            }

            float v=cumul * ldexpf(rand()&0xfffff, -20);
            int c=0;
            while (c+1<candidates.size() && candidates[c+1].cumul<=v) c++;

            //printf("total=%f  v=%f  c=%d\n", cumul, v, c);

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


std::vector<Scale> compute_scales_for_chords(const std::vector<Chord>& chords)
{
    const int n=chords.size();

    struct node_t {
        Scale   scale;
        int     nonchordtones;
        int     cost;
        int     back;
    };

    node_t* nodes=new node_t[n*7];
    for (int i=0;i<n;i++) {
        for (int j=0;j<7;j++) {
            nodes[i*7+j].scale=Scale(chords[i].notes[0], Scale::Mode(j));
            nodes[i*7+j].nonchordtones=0;

            for (int k=0;k<6 && chords[i].notes[k];k++)
                if (!nodes[i*7+j].scale.contains(chords[i].notes[k]))
                    nodes[i*7+j].nonchordtones++;
        }
    }

    Scale alleged_tonic_scale(chords[0].notes[0], chords[0].quality==Chord::Quality::Minor ? Scale::Mode::Aeolian : Scale::Mode::Ionian);

    for (int j=0;j<7;j++) {
        nodes[j].cost=nodes[j].nonchordtones*8 + Scale::distance(alleged_tonic_scale, nodes[j].scale);
        nodes[j].back=-1;
    }

    for (int i=1;i<n;i++) {
        for (int j=0;j<7;j++) {
            nodes[i*7+j].cost=INT_MAX;
            nodes[i*7+j].back=0;

            for (int k=0;k<7;k++) {
                int cost=nodes[(i-1)*7+k].cost + nodes[i*7+j].nonchordtones*8;
                int dist=Scale::distance(nodes[(i-1)*7+k].scale, nodes[i*7+j].scale);
                if (dist)
                    cost+=dist+1;
                
                if (cost<nodes[i*7+j].cost) {
                    nodes[i*7+j].cost=cost;
                    nodes[i*7+j].back=k;
                }
            }
        }
    }


    int bestscale=0;
    for (int j=1;j<7;j++)
        if (nodes[(n-1)*7+j].cost < nodes[(n-1)*7+bestscale].cost)
            bestscale=j;
        
    std::vector<Scale> scales;

    for (int i=n-1;i>=0;bestscale=nodes[i--*7+bestscale].back)
        scales.push_back(nodes[i*7+bestscale].scale);

    std::reverse(scales.begin(), scales.end());

    return scales;
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


    EnsembleParser parseensemble;
    std::ifstream ensemblestream;
    ensemblestream.open("ensembles/strings");
    Ensemble ensemble=parseensemble(ensemblestream);


    std::vector<Ensemble::Voicing> voiceleading=compute_voice_leading(ensemble, chords);

    std::vector<Note> melody=improvise_melody(chords);
    melody=improvise_passing_notes(melody, chords);

    for (int i=0;i<melody.size();i++)
        std::cout << (i&1 ? "\e[0m" : "\e[1m") << melody[i].get_name() << "\e[0m" << std::endl;


    auto scales=compute_scales_for_chords(chords);

    Sequencer seq;

    for (int i=0;i<voiceleading.size();i++) {
        ensemble.print_harmony_voicing(chords[i], scales[i], voiceleading[i]);
        
        for (int j=0;j<voiceleading[i].get_voice_count();j++)
            seq.sequence_note(ensemble.get_harmony_voice(j), 2.0f*i, voiceleading[i][j]);
    }

    for (int j=0;j<5;j++)
        seq.sequence_pause(ensemble.get_harmony_voice(j), voiceleading.size()*2.0f);

    const auto& melody_voice=ensemble.get_melody_voice(0);

    const float melody_timing[4]={ 0.0f, 0.75f, 1.0f, 1.75f };
    for (int i=0;i<melody.size();i++)
        seq.sequence_note(melody_voice, (i&~3)*0.5f + melody_timing[i&3], melody[i]);
    
    seq.sequence_pause(melody_voice, melody.size()*0.5f);

    if (opt_play) {
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

            ensemble.init_midi_programs(midiout);

            seq.play(midiout);
        }
        catch (const RtMidiError& err) {
            err.printMessage();
        }
    }

    poptFreeContext(pctx);

    return 0;
}
