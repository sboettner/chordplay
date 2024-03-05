#include <algorithm>
#include <fstream>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <popt.h>
#include "chordparser.h"
#include "ensembleparser.h"
#include "scale.h"
#include "midi.h"

int opt_play=0;
int opt_improvise=0;
int opt_bpm=120;

const char* opt_transpose_to=nullptr;
int opt_transpose_by=0;

poptOption option_table[]={
    { NULL, 'p', POPT_ARG_NONE,     &opt_play,          0, "Play using MIDI output", NULL },
    { NULL, 'i', POPT_ARG_NONE,     &opt_improvise,     0, "Improvise a melody", NULL },
    { NULL, 'B', POPT_ARG_INT,      &opt_bpm,           0,  "Set tempo (beats per minute)", "BPM" },
    { NULL, 't', POPT_ARG_STRING,   &opt_transpose_to,  0, "Transpose such that the progression starts with a chord rooted on the given note", "NOTE" },
    { NULL, 'T', POPT_ARG_INT,      &opt_transpose_by,  0, "Play the progression transposed by the given number of semitones", "SEMITONES" },
    POPT_AUTOHELP
    POPT_TABLEEND
};


struct Bar {
    Chord               chord;
    Scale               scale;
    Ensemble::Voicing   voicing;
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


void compute_voice_leading(const Ensemble& ensemble, std::vector<Bar>& bars)
{
    struct pathnode_t {
        Ensemble::Voicing   voicing;
        int                 back;
        int                 cost;
    };

    std::vector<std::vector<pathnode_t>> pathnodes;

    for (const Bar& bar: bars) {
        std::vector<pathnode_t> voicings;

        for (Ensemble::Voicing& v: ensemble.enumerate_harmony_voicings(bar.chord))
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

    while (i>=0) {
        bars[i].voicing=std::move(pathnodes[i][j].voicing);
        j=pathnodes[i--][j].back;
    }
}


std::vector<Note> improvise_melody(const std::vector<Bar>& bars)
{
    std::vector<Note> melody;
    std::vector<std::vector<Note>> noteset;

    const int n=bars.size()*2 - 1;

    for (int i=0;i<n;i++) {
        const Chord& chord=bars[i/2].chord;

        melody.emplace_back(chord.notes[0], 5);

        std::vector<Note> notes;
        for (int j=0;j<6 && chord.notes[j];j++) {
            notes.emplace_back(chord.notes[j], 4);
            notes.emplace_back(chord.notes[j], 5);
            notes.emplace_back(chord.notes[j], 6);
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


std::vector<Note> improvise_passing_notes(const std::vector<Note>& in_melody, const std::vector<Bar>& bars)
{
    std::vector<Note> melody;

    for (int i=0;i+1<in_melody.size();i++) {
        const Scale& scale=bars[i/2].scale;

        melody.push_back(in_melody[i]);

        int8_t prev=scale.to_scale(in_melody[i]);
        int8_t next=scale.to_scale(in_melody[i+1]);

        switch (next-prev) {
        case -3:
        case -2:
            melody.push_back(scale(prev-1));
            break;
        case -4:
        case -1:
            melody.push_back(scale(prev-2));
            break;
        case 1:
        case 4:
            melody.push_back(scale(prev+2));
            break;
        case 0:
        case 2:
        case 3:
            melody.push_back(scale(prev+1));
            break;
        default:
            melody.push_back(in_melody[i]);
        }
    }

    melody.push_back(in_melody.back());

    return melody;
}


void compute_scales_for_chords(std::vector<Bar>& bars)
{
    const int n=bars.size();

    struct node_t {
        Scale   scale;
        int     nonchordtones;
        int     cost;
        int     back;
    };

    node_t* nodes=new node_t[n*7];
    for (int i=0;i<n;i++) {
        const Chord& chord=bars[i].chord;

        for (int j=0;j<7;j++) {
            nodes[i*7+j].scale=Scale(chord.notes[0], Scale::Mode(j));
            nodes[i*7+j].nonchordtones=0;

            for (int k=0;k<6 && chord.notes[k];k++)
                if (!nodes[i*7+j].scale.contains(chord.notes[k]))
                    nodes[i*7+j].nonchordtones++;
        }
    }

    Scale alleged_tonic_scale(bars[0].chord.notes[0], bars[0].chord.quality==Chord::Quality::Minor ? Scale::Mode::Aeolian : Scale::Mode::Ionian);

    for (int j=0;j<7;j++) {
        nodes[j].cost=nodes[j].nonchordtones*16 + Scale::distance(alleged_tonic_scale, nodes[j].scale);
        nodes[j].back=-1;
    }

    for (int i=1;i<n;i++) {
        for (int j=0;j<7;j++) {
            nodes[i*7+j].cost=INT_MAX;
            nodes[i*7+j].back=0;

            for (int k=0;k<7;k++) {
                int cost=nodes[(i-1)*7+k].cost + nodes[i*7+j].nonchordtones*16;
                int dist=Scale::distance(nodes[(i-1)*7+k].scale, nodes[i*7+j].scale);
                if (dist)
                    cost+=dist+1;

                // check if we already had the same chord earlier in the progression -- if so, try to use the same scale
                for (int p=i-1, q=k; p>=0; q=nodes[p--*7+q].back) {
                    if (bars[i].chord==bars[p].chord) {
                        if (nodes[i*7+j].scale!=nodes[p*7+q].scale)
                            cost+=3;
                        
                        break;
                    }
                }
                
                if (cost<nodes[i*7+j].cost) {
                    nodes[i*7+j].cost=cost;
                    nodes[i*7+j].back=k;
                }
            }
        }
    }


    int bestscale=0;
    for (int j=0;j<7;j++)
        if (nodes[(n-1)*7+j].cost < nodes[(n-1)*7+bestscale].cost)
            bestscale=j;
    
    for (int i=n-1;i>=0;bestscale=nodes[i--*7+bestscale].back)
        bars[i].scale=nodes[i*7+bestscale].scale;
}


Sequencer* seq=nullptr;

void break_handler(int sig)
{
    signal(SIGINT, SIG_DFL);

    if (seq)
        seq->stop();
}


int main(int argc, const char* argv[])
{
    poptContext pctx=poptGetContext(NULL, argc, argv, option_table, 0);

    while (poptGetNextOpt(pctx)>=0);

    ChordParser parsechord;
    std::vector<Bar> bars;

    while (const char* arg=poptGetArg(pctx)) {
        auto chord=parsechord(arg);
        if (!chord.has_value()) {
            printf("Error: invalid chord '%s'\n", arg);
            return 1;
        }

        Bar bar;
        bar.chord=*chord;

        bars.push_back(std::move(bar));
    }

    if (bars.empty()) {
        printf("Error: no chords given\n");
        return 1;
    }

    if (opt_transpose_to) {
        Interval trans=NoteClass(opt_transpose_to) - bars[0].chord.notes[0];
        for (auto& b: bars)
            b.chord+=trans;
    }

    EnsembleParser parseensemble;
    std::ifstream ensemblestream;
    ensemblestream.open("ensembles/strings");
    Ensemble ensemble=parseensemble(ensemblestream);


    compute_voice_leading(ensemble, bars);
    compute_scales_for_chords(bars);

    for (int i=0;i<bars.size();i++)
        ensemble.print_harmony_voicing(bars[i].chord, bars[i].scale, bars[i].voicing);
    
    if (opt_play) {
        signal(SIGINT, break_handler);

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

            rtmidiout.openPort(midiport);

            MidiOut midiout(rtmidiout);

            seq=new Sequencer(midiout, opt_bpm, opt_transpose_by);

            for (int i=0;i<ensemble.get_harmony_voice_count();i++) {
                const auto& voice=ensemble.get_harmony_voice(i);

                auto* track=seq->add_track(voice.midi_channel, voice.midi_program);

                for (int j=0;j<bars.size();j++)
                    track->append_note(4.0f*j, bars[j].voicing[i], voice.midi_velocity);

                track->append_pause(bars.size()*4.0f);
            }

            if (opt_improvise) {
                std::vector<Note> melody=improvise_melody(bars);
                melody=improvise_passing_notes(melody, bars);

                const auto& melody_voice=ensemble.get_melody_voice(0);
                auto* melody_track=seq->add_track(melody_voice.midi_channel, melody_voice.midi_program);

                const float melody_timing[4]={ 0.0f, 1.5f, 2.0f, 3.5f };
                for (int i=0;i<melody.size();i++)
                    melody_track->append_note((i&~3)*1.0f + melody_timing[i&3], melody[i], melody_voice.midi_velocity);
                
                melody_track->append_pause(melody.size()*1.0f);
            }

            ensemble.init_midi_programs(midiout);

            seq->play();
        }
        catch (const RtMidiError& err) {
            err.printMessage();
        }
    }

    poptFreeContext(pctx);

    return 0;
}
