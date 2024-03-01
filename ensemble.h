#ifndef INCLUDE_ENSEMBLE_H
#define INCLUDE_ENSEMBLE_H

#include <vector>
#include "note.h"

class Chord;
class Scale;
class MidiOut;


class Ensemble {
public:
    struct Voice {
        enum class Role:uint8_t {
            Bass,
            Harmony,
            Melody
        };

        Role    role;
        int8_t  midi_channel;
        int8_t  midi_program;
        int8_t  midi_velocity;
        Note    range_low;
        Note    range_high;
        int8_t  color;
        int8_t  id;
    };


    class Voicing {
        int     numvoices;
        Note*   notes;

    public:
        Voicing() = delete;
        
        explicit Voicing(int numvoices);
        Voicing(const Voicing&) = delete;
        Voicing(Voicing&&);
        ~Voicing();

        Voicing& operator=(const Voicing&) = delete;
        Voicing& operator=(Voicing&&);

        int get_voice_count() const
        {
            return numvoices;
        }

        Note& operator[](int i)
        {
            return notes[i];
        }

        const Note& operator[](int i) const
        {
            return notes[i];
        }
    };


    void add_voice(const Voice&);

    const Voice& get_harmony_voice(int i) const
    {
        return harmony_voices[i];
    }
    
    int get_harmony_voice_count() const
    {
        return harmony_voices.size();
    }

    const Voice& get_melody_voice(int i) const
    {
        return melody_voices[i];
    }
    
    int get_melody_voice_count() const
    {
        return melody_voices.size();
    }

    void init_midi_programs(MidiOut&) const;

    std::vector<Voicing> enumerate_harmony_voicings(const Chord&) const;

    void print_harmony_voicing(const Chord&, const Scale&, const Voicing&) const;

private:
    std::vector<Voice>  harmony_voices;
    std::vector<Voice>  melody_voices;
};

#endif
